#!/usr/bin/env python3
"""
Unreal Engine Documentation Scraper
Developer: Marcus Daley
Project: WizardJam (Portfolio Capstone)
Created: January 25, 2026

Scrapes Unreal Engine documentation for offline browsing and Claude context.
Converts HTML documentation to clean Markdown format.

Usage:
    python ue_docs_scraper.py --url "https://dev.epicgames.com/documentation/en-us/unreal-engine/behavior-tree-in-unreal-engine"
    python ue_docs_scraper.py --topic "behavior-tree"
    python ue_docs_scraper.py --topic "ai-perception" --depth 2
    python ue_docs_scraper.py --list-topics
"""

import argparse
import hashlib
import json
import os
import re
import sys
import time
from datetime import datetime
from pathlib import Path
from typing import Optional
from urllib.parse import urljoin, urlparse

try:
    import requests
    from bs4 import BeautifulSoup
except ImportError:
    print("Required packages not installed. Run:")
    print("  pip install requests beautifulsoup4")
    sys.exit(1)


# Configuration
BASE_URL = "https://dev.epicgames.com/documentation/en-us/unreal-engine"
OUTPUT_DIR = Path(__file__).parent / "scraped_docs"
CACHE_DIR = Path(__file__).parent / ".cache"
REQUEST_DELAY = 1.0  # seconds between requests (be respectful)
MAX_RETRIES = 3
USER_AGENT = "WizardJam-DocScraper/1.0 (Educational/Portfolio Project)"

# Common UE5 documentation topics for quick access
TOPIC_SHORTCUTS = {
    "behavior-tree": "behavior-tree-in-unreal-engine",
    "bt": "behavior-tree-in-unreal-engine",
    "ai": "artificial-intelligence",
    "ai-perception": "ai-perception-in-unreal-engine",
    "perception": "ai-perception-in-unreal-engine",
    "blackboard": "blackboard-in-unreal-engine",
    "eqs": "environment-query-system-in-unreal-engine",
    "navigation": "navigation-system-in-unreal-engine",
    "nav-mesh": "navigation-mesh-in-unreal-engine",
    "gameplay-ability": "gameplay-ability-system-for-unreal-engine",
    "gas": "gameplay-ability-system-for-unreal-engine",
    "enhanced-input": "enhanced-input-in-unreal-engine",
    "input": "enhanced-input-in-unreal-engine",
    "niagara": "niagara-visual-effects",
    "vfx": "niagara-visual-effects",
    "materials": "unreal-engine-materials",
    "blueprints": "blueprints-visual-scripting-in-unreal-engine",
    "bp": "blueprints-visual-scripting-in-unreal-engine",
    "cpp": "programming-with-cplusplus-in-unreal-engine",
    "c++": "programming-with-cplusplus-in-unreal-engine",
    "umg": "umg-ui-designer-in-unreal-engine",
    "ui": "umg-ui-designer-in-unreal-engine",
    "widgets": "creating-widgets-in-unreal-engine",
    "animation": "skeletal-mesh-animation-system-in-unreal-engine",
    "physics": "physics-in-unreal-engine",
    "collision": "collision-in-unreal-engine",
    "networking": "networking-and-multiplayer-in-unreal-engine",
    "replication": "property-replication-in-unreal-engine",
    "packaging": "packaging-unreal-engine-projects",
    "build": "packaging-unreal-engine-projects",
}


class UEDocsScraper:
    """Scrapes and converts UE documentation to markdown."""

    def __init__(self, output_dir: Optional[Path] = None, use_cache: bool = True):
        self.output_dir = output_dir or OUTPUT_DIR
        self.cache_dir = CACHE_DIR
        self.use_cache = use_cache
        self.session = requests.Session()
        self.session.headers.update({"User-Agent": USER_AGENT})

        # Create directories
        self.output_dir.mkdir(parents=True, exist_ok=True)
        if use_cache:
            self.cache_dir.mkdir(parents=True, exist_ok=True)

    def _get_cache_path(self, url: str) -> Path:
        """Generate cache file path from URL."""
        url_hash = hashlib.md5(url.encode()).hexdigest()
        return self.cache_dir / f"{url_hash}.html"

    def _fetch_page(self, url: str) -> Optional[str]:
        """Fetch a page with caching and retry logic."""
        # Check cache first
        if self.use_cache:
            cache_path = self._get_cache_path(url)
            if cache_path.exists():
                print(f"  [CACHE] {url}")
                return cache_path.read_text(encoding="utf-8")

        # Fetch from web
        for attempt in range(MAX_RETRIES):
            try:
                print(f"  [FETCH] {url}")
                response = self.session.get(url, timeout=30)
                response.raise_for_status()

                # Cache the response
                if self.use_cache:
                    cache_path.write_text(response.text, encoding="utf-8")

                time.sleep(REQUEST_DELAY)  # Rate limiting
                return response.text

            except requests.RequestException as e:
                print(f"  [ERROR] Attempt {attempt + 1}/{MAX_RETRIES}: {e}")
                if attempt < MAX_RETRIES - 1:
                    time.sleep(2 ** attempt)  # Exponential backoff

        return None

    def _html_to_markdown(self, html: str, base_url: str) -> str:
        """Convert HTML documentation to clean Markdown."""
        soup = BeautifulSoup(html, "html.parser")

        # Find main content area (UE docs structure)
        content = soup.find("main") or soup.find("article") or soup.find(class_="content")
        if not content:
            content = soup.body or soup

        # Remove unwanted elements
        for element in content.find_all(["script", "style", "nav", "footer", "header", "aside"]):
            element.decompose()

        # Remove navigation breadcrumbs and sidebars
        for selector in [".breadcrumb", ".sidebar", ".toc", ".navigation", "[role='navigation']"]:
            for element in content.select(selector):
                element.decompose()

        markdown_lines = []
        self._process_element(content, markdown_lines, base_url)

        # Clean up the markdown
        markdown = "\n".join(markdown_lines)
        markdown = re.sub(r"\n{3,}", "\n\n", markdown)  # Remove excessive newlines
        markdown = markdown.strip()

        return markdown

    def _process_element(self, element, lines: list, base_url: str, depth: int = 0):
        """Recursively process HTML elements to markdown."""
        if element.name is None:  # Text node
            text = element.get_text().strip()
            if text:
                lines.append(text)
            return

        tag = element.name.lower()

        # Headings
        if tag in ["h1", "h2", "h3", "h4", "h5", "h6"]:
            level = int(tag[1])
            text = element.get_text().strip()
            if text:
                lines.append("")
                lines.append(f"{'#' * level} {text}")
                lines.append("")
            return

        # Paragraphs
        if tag == "p":
            text = self._get_inline_text(element, base_url)
            if text.strip():
                lines.append("")
                lines.append(text)
                lines.append("")
            return

        # Code blocks
        if tag == "pre":
            code = element.get_text()
            lang = ""
            code_elem = element.find("code")
            if code_elem and code_elem.get("class"):
                for cls in code_elem.get("class", []):
                    if cls.startswith("language-"):
                        lang = cls.replace("language-", "")
                        break
                    elif cls in ["cpp", "c++", "python", "blueprint", "ini", "json"]:
                        lang = cls
                        break
            lines.append("")
            lines.append(f"```{lang}")
            lines.append(code.strip())
            lines.append("```")
            lines.append("")
            return

        # Inline code
        if tag == "code" and element.parent.name != "pre":
            lines.append(f"`{element.get_text()}`")
            return

        # Lists
        if tag in ["ul", "ol"]:
            lines.append("")
            for i, li in enumerate(element.find_all("li", recursive=False)):
                prefix = f"{i + 1}." if tag == "ol" else "-"
                text = self._get_inline_text(li, base_url)
                lines.append(f"{prefix} {text}")
            lines.append("")
            return

        # Tables
        if tag == "table":
            self._process_table(element, lines)
            return

        # Links (standalone)
        if tag == "a":
            href = element.get("href", "")
            text = element.get_text().strip()
            if href and text:
                if not href.startswith(("http://", "https://")):
                    href = urljoin(base_url, href)
                lines.append(f"[{text}]({href})")
            return

        # Images
        if tag == "img":
            alt = element.get("alt", "Image")
            src = element.get("src", "")
            if src:
                if not src.startswith(("http://", "https://")):
                    src = urljoin(base_url, src)
                lines.append(f"![{alt}]({src})")
            return

        # Blockquotes / callouts
        if tag == "blockquote" or (tag == "div" and "callout" in " ".join(element.get("class", []))):
            text = element.get_text().strip()
            if text:
                lines.append("")
                for line in text.split("\n"):
                    if line.strip():
                        lines.append(f"> {line.strip()}")
                lines.append("")
            return

        # Divs and other containers - recurse into children
        if tag in ["div", "section", "article", "main", "span", "body"]:
            for child in element.children:
                self._process_element(child, lines, base_url, depth + 1)
            return

        # Default: just get text
        text = element.get_text().strip()
        if text:
            lines.append(text)

    def _get_inline_text(self, element, base_url: str) -> str:
        """Get text with inline formatting (bold, italic, code, links)."""
        parts = []
        for child in element.children:
            if child.name is None:
                parts.append(child.get_text())
            elif child.name == "strong" or child.name == "b":
                parts.append(f"**{child.get_text()}**")
            elif child.name == "em" or child.name == "i":
                parts.append(f"*{child.get_text()}*")
            elif child.name == "code":
                parts.append(f"`{child.get_text()}`")
            elif child.name == "a":
                href = child.get("href", "")
                text = child.get_text()
                if href:
                    if not href.startswith(("http://", "https://")):
                        href = urljoin(base_url, href)
                    parts.append(f"[{text}]({href})")
                else:
                    parts.append(text)
            else:
                parts.append(child.get_text())
        return "".join(parts).strip()

    def _process_table(self, table, lines: list):
        """Convert HTML table to Markdown table."""
        lines.append("")

        rows = table.find_all("tr")
        if not rows:
            return

        # Process header
        header_row = rows[0]
        headers = [th.get_text().strip() for th in header_row.find_all(["th", "td"])]
        if headers:
            lines.append("| " + " | ".join(headers) + " |")
            lines.append("| " + " | ".join(["---"] * len(headers)) + " |")

        # Process body rows
        for row in rows[1:]:
            cells = [td.get_text().strip() for td in row.find_all(["td", "th"])]
            if cells:
                lines.append("| " + " | ".join(cells) + " |")

        lines.append("")

    def _extract_links(self, html: str, base_url: str) -> list:
        """Extract documentation links from a page for crawling."""
        soup = BeautifulSoup(html, "html.parser")
        links = []

        for a in soup.find_all("a", href=True):
            href = a["href"]
            # Only follow UE documentation links
            if "/documentation/en-us/unreal-engine" in href or href.startswith("/"):
                full_url = urljoin(base_url, href)
                if "dev.epicgames.com/documentation" in full_url:
                    # Remove anchors and query strings
                    full_url = full_url.split("#")[0].split("?")[0]
                    if full_url not in links:
                        links.append(full_url)

        return links

    def scrape_page(self, url: str) -> Optional[dict]:
        """Scrape a single documentation page."""
        html = self._fetch_page(url)
        if not html:
            return None

        soup = BeautifulSoup(html, "html.parser")

        # Extract title
        title = "Untitled"
        title_elem = soup.find("h1") or soup.find("title")
        if title_elem:
            title = title_elem.get_text().strip()
            title = title.replace(" | Unreal Engine Documentation", "")

        # Convert to markdown
        markdown = self._html_to_markdown(html, url)

        # Extract child links
        links = self._extract_links(html, url)

        return {
            "url": url,
            "title": title,
            "markdown": markdown,
            "links": links,
            "scraped_at": datetime.now().isoformat(),
        }

    def scrape_topic(self, topic: str, depth: int = 1) -> list:
        """Scrape a topic and optionally follow links to specified depth."""
        # Resolve topic shortcut
        topic_slug = TOPIC_SHORTCUTS.get(topic.lower(), topic)
        url = f"{BASE_URL}/{topic_slug}"

        print(f"\nScraping topic: {topic}")
        print(f"URL: {url}")
        print(f"Depth: {depth}")

        scraped = []
        visited = set()
        to_visit = [(url, 0)]

        while to_visit:
            current_url, current_depth = to_visit.pop(0)

            if current_url in visited:
                continue
            if current_depth > depth:
                continue

            visited.add(current_url)

            result = self.scrape_page(current_url)
            if result:
                scraped.append(result)

                # Add child links if not at max depth
                if current_depth < depth:
                    for link in result["links"]:
                        if link not in visited:
                            to_visit.append((link, current_depth + 1))

        return scraped

    def save_markdown(self, data: dict, filename: Optional[str] = None) -> Path:
        """Save scraped data as a Markdown file."""
        if not filename:
            # Generate filename from title
            filename = re.sub(r"[^\w\s-]", "", data["title"])
            filename = re.sub(r"\s+", "-", filename).lower()
            filename = f"{filename}.md"

        filepath = self.output_dir / filename

        # Create markdown with metadata header
        content = f"""---
title: {data['title']}
source: {data['url']}
scraped_at: {data['scraped_at']}
---

# {data['title']}

{data['markdown']}
"""

        filepath.write_text(content, encoding="utf-8")
        print(f"  [SAVED] {filepath}")
        return filepath

    def save_combined(self, results: list, filename: str = "ue_docs_combined.md") -> Path:
        """Save multiple pages as a single combined document."""
        filepath = self.output_dir / filename

        sections = []
        toc = ["# Table of Contents\n"]

        for i, data in enumerate(results, 1):
            anchor = re.sub(r"[^\w\s-]", "", data["title"]).lower().replace(" ", "-")
            toc.append(f"{i}. [{data['title']}](#{anchor})")

            sections.append(f"""
---

<a name="{anchor}"></a>

# {data['title']}

**Source:** {data['url']}

{data['markdown']}
""")

        content = f"""---
title: Unreal Engine Documentation (Combined)
scraped_at: {datetime.now().isoformat()}
page_count: {len(results)}
---

{chr(10).join(toc)}

{"".join(sections)}
"""

        filepath.write_text(content, encoding="utf-8")
        print(f"\n[COMBINED] Saved {len(results)} pages to {filepath}")
        return filepath

    def create_claude_context(self, results: list, filename: str = "UE_CONTEXT.md") -> Path:
        """Create a condensed context file optimized for Claude."""
        filepath = self.output_dir / filename

        # Extract key information, reduce verbosity
        sections = []

        for data in results:
            # Condense the markdown - remove images, simplify
            condensed = data["markdown"]
            condensed = re.sub(r"!\[.*?\]\(.*?\)", "", condensed)  # Remove images
            condensed = re.sub(r"\n{3,}", "\n\n", condensed)  # Reduce whitespace

            sections.append(f"""## {data['title']}

{condensed}
""")

        content = f"""# Unreal Engine Reference Documentation

This file contains scraped UE5 documentation for offline Claude context.
Generated: {datetime.now().isoformat()}
Pages: {len(results)}

---

{"---".join(sections)}
"""

        filepath.write_text(content, encoding="utf-8")
        print(f"\n[CLAUDE CONTEXT] Saved to {filepath}")
        return filepath


def main():
    parser = argparse.ArgumentParser(
        description="Scrape Unreal Engine documentation for offline use",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s --url "https://dev.epicgames.com/documentation/en-us/unreal-engine/behavior-tree-in-unreal-engine"
  %(prog)s --topic behavior-tree
  %(prog)s --topic ai --depth 2
  %(prog)s --topic bt --depth 1 --combined
  %(prog)s --list-topics
        """
    )

    parser.add_argument("--url", help="Specific documentation URL to scrape")
    parser.add_argument("--topic", help="Topic shortcut or slug to scrape")
    parser.add_argument("--depth", type=int, default=1, help="How many levels of links to follow (default: 1)")
    parser.add_argument("--combined", action="store_true", help="Save all pages to a single combined file")
    parser.add_argument("--claude-context", action="store_true", help="Create condensed context file for Claude")
    parser.add_argument("--output-dir", help="Custom output directory")
    parser.add_argument("--no-cache", action="store_true", help="Disable caching")
    parser.add_argument("--list-topics", action="store_true", help="List available topic shortcuts")

    args = parser.parse_args()

    if args.list_topics:
        print("\nAvailable topic shortcuts:")
        print("-" * 50)
        for shortcut, slug in sorted(TOPIC_SHORTCUTS.items()):
            print(f"  {shortcut:20} -> {slug}")
        print(f"\nUsage: python {sys.argv[0]} --topic <shortcut>")
        return

    if not args.url and not args.topic:
        parser.print_help()
        print("\nError: Either --url or --topic is required")
        sys.exit(1)

    # Initialize scraper
    output_dir = Path(args.output_dir) if args.output_dir else None
    scraper = UEDocsScraper(output_dir=output_dir, use_cache=not args.no_cache)

    print(f"\n{'=' * 60}")
    print("Unreal Engine Documentation Scraper")
    print(f"{'=' * 60}")

    results = []

    if args.url:
        # Scrape single URL
        result = scraper.scrape_page(args.url)
        if result:
            results.append(result)
    elif args.topic:
        # Scrape topic with depth
        results = scraper.scrape_topic(args.topic, depth=args.depth)

    if not results:
        print("\nNo pages scraped successfully.")
        sys.exit(1)

    print(f"\n{'=' * 60}")
    print(f"Scraped {len(results)} page(s)")
    print(f"{'=' * 60}")

    # Save results
    if args.combined or args.claude_context:
        if args.combined:
            scraper.save_combined(results)
        if args.claude_context:
            scraper.create_claude_context(results)
    else:
        # Save individual files
        for result in results:
            scraper.save_markdown(result)

    print(f"\nOutput directory: {scraper.output_dir}")
    print("Done!")


if __name__ == "__main__":
    main()
