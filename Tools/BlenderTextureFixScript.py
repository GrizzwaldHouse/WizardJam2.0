"""
Blender UV Island Margin Fix Script
Author: Marcus Daley (WizardJam Project)
Date: January 25, 2026
Purpose: Fixes texture bleeding on Meshy.ai generated models by adding margin to UV islands

USAGE:
1. Open Blender 3.6+
2. Import the Meshy.ai FBX file (File -> Import -> FBX)
3. Open Scripting workspace
4. Load this script
5. Run the script (Alt+P or click Run Script button)
6. Export fixed model (File -> Export -> FBX)

TESTED WITH:
- Rainbow Quidditch Stadium (16,506 faces, 17,888 vertices)
- Blender 3.6, 4.0, 4.1
"""

import bpy
import bmesh


def select_all_mesh_objects():
    """
    Selects all mesh objects in the current scene.
    Returns list of selected mesh objects.
    """
    mesh_objects = [obj for obj in bpy.data.objects if obj.type == 'MESH']

    # Deselect all first
    bpy.ops.object.select_all(action='DESELECT')

    # Select all mesh objects
    for obj in mesh_objects:
        obj.select_set(True)

    if mesh_objects:
        bpy.context.view_layer.objects.active = mesh_objects[0]

    return mesh_objects


def fix_uv_bleeding(margin_pixels=4, texture_size=2048):
    """
    Adds margin to UV islands to prevent texture bleeding at seams.

    Args:
        margin_pixels: Number of pixels of margin to add (default: 4)
        texture_size: Texture resolution in pixels (default: 2048)
    """

    print(f"\n{'='*60}")
    print(f"Starting UV Bleeding Fix")
    print(f"Margin: {margin_pixels}px at {texture_size}x{texture_size} texture")
    print(f"{'='*60}\n")

    mesh_objects = select_all_mesh_objects()

    if not mesh_objects:
        print("ERROR: No mesh objects found in scene!")
        return False

    print(f"Found {len(mesh_objects)} mesh object(s)")

    margin_uv = margin_pixels / texture_size
    print(f"UV margin value: {margin_uv:.6f}")

    for i, obj in enumerate(mesh_objects, 1):
        print(f"\n[{i}/{len(mesh_objects)}] Processing: {obj.name}")
        bpy.context.view_layer.objects.active = obj

        if bpy.context.mode != 'OBJECT':
            bpy.ops.object.mode_set(mode='OBJECT')

        bpy.ops.object.mode_set(mode='EDIT')
        bpy.ops.mesh.select_all(action='SELECT')

        if not obj.data.uv_layers:
            print(f"  WARNING: {obj.name} has no UV map! Skipping...")
            bpy.ops.object.mode_set(mode='OBJECT')
            continue

        uv_layer = obj.data.uv_layers.active
        print(f"  Active UV layer: {uv_layer.name}")

        try:
            bpy.ops.uv.select_all(action='SELECT')
            bpy.ops.uv.pack_islands(rotate=True, margin=margin_uv)
            print(f"  ✓ UV islands packed with {margin_pixels}px margin")
        except Exception as e:
            print(f"  ERROR packing UVs for {obj.name}: {e}")
            continue

        bpy.ops.object.mode_set(mode='OBJECT')

    print(f"\n{'='*60}")
    print(f"UV Bleeding Fix Complete!")
    print(f"Export as: SM_QuidditchStadium_Rainbow_Fixed.fbx")
    print(f"{'='*60}\n")

    return True


if __name__ == "__main__":
    fix_uv_bleeding(margin_pixels=4, texture_size=2048)
