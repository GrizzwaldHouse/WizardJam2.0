# UE Documentation Scraper

Scrapes Unreal Engine documentation for offline browsing and Claude context.

## Setup

```bash
cd Tools/UEDocsScraper
pip install -r requirements.txt
```

## Usage

### Scrape by Topic (Recommended)

```bash
# Quick topic shortcuts
python ue_docs_scraper.py --topic behavior-tree
python ue_docs_scraper.py --topic ai-perception
python ue_docs_scraper.py --topic blackboard

# Follow links to depth 2
python ue_docs_scraper.py --topic bt --depth 2

# Create combined file for easier reading
python ue_docs_scraper.py --topic ai --depth 2 --combined

# Create condensed Claude context file
python ue_docs_scraper.py --topic gas --claude-context
```

### Scrape Specific URL

```bash
python ue_docs_scraper.py --url "https://dev.epicgames.com/documentation/en-us/unreal-engine/behavior-tree-in-unreal-engine"
```

### List Available Topic Shortcuts

```bash
python ue_docs_scraper.py --list-topics
```

## Output

Files are saved to `Tools/UEDocsScraper/scraped_docs/`:

- Individual markdown files per page (default)
- `ue_docs_combined.md` (with `--combined`)
- `UE_CONTEXT.md` (with `--claude-context`)

## Adding to Claude Context

After scraping, copy the relevant `.md` file to your project and reference it in `CLAUDE.md`:

```markdown
# In CLAUDE.md
See `Docs/UE_BehaviorTree.md` for Behavior Tree reference.
```

Or paste the condensed content directly into your CLAUDE.md file.

## Topic Shortcuts

| Shortcut | Full Slug |
|----------|-----------|
| bt, behavior-tree | behavior-tree-in-unreal-engine |
| ai | artificial-intelligence |
| ai-perception, perception | ai-perception-in-unreal-engine |
| blackboard | blackboard-in-unreal-engine |
| eqs | environment-query-system-in-unreal-engine |
| navigation, nav-mesh | navigation-system-in-unreal-engine |
| gas, gameplay-ability | gameplay-ability-system-for-unreal-engine |
| enhanced-input, input | enhanced-input-in-unreal-engine |
| niagara, vfx | niagara-visual-effects |
| bp, blueprints | blueprints-visual-scripting-in-unreal-engine |
| cpp, c++ | programming-with-cplusplus-in-unreal-engine |
| umg, ui | umg-ui-designer-in-unreal-engine |
| widgets | creating-widgets-in-unreal-engine |
| animation | skeletal-mesh-animation-system-in-unreal-engine |
| physics | physics-in-unreal-engine |
| collision | collision-in-unreal-engine |
| networking | networking-and-multiplayer-in-unreal-engine |
| replication | property-replication-in-unreal-engine |
| packaging, build | packaging-unreal-engine-projects |

## Caching

Pages are cached in `.cache/` directory. Use `--no-cache` to force fresh fetches.

## Rate Limiting

The scraper waits 1 second between requests to be respectful of Epic's servers.
