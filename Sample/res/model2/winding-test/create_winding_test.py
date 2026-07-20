from pathlib import Path

import bpy


ROOT_DIRECTORY = Path(__file__).resolve().parent

VERTICES = (
    (-0.5, -0.5, -0.5),
    (0.5, -0.5, -0.5),
    (0.5, 0.5, -0.5),
    (-0.5, 0.5, -0.5),
    (-0.5, -0.5, 0.5),
    (0.5, -0.5, 0.5),
    (0.5, 0.5, 0.5),
    (-0.5, 0.5, 0.5),
)

# Blenderで立方体の外側から見たときに反時計回りとなる面です。
COUNTERCLOCKWISE_FACES = (
    (0, 3, 2, 1),
    (4, 5, 6, 7),
    (0, 1, 5, 4),
    (3, 7, 6, 2),
    (0, 4, 7, 3),
    (1, 2, 6, 5),
)


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)


def create_material(name, color):
    material = bpy.data.materials.new(name=name)
    material.diffuse_color = color
    material["_x_face_color"] = color
    material["_x_power"] = 500.0
    material["_x_specular"] = (1.0, 1.0, 1.0)
    return material


def export_cube(directory_name, object_name, faces, color):
    clear_scene()

    output_directory = ROOT_DIRECTORY / directory_name
    output_directory.mkdir(parents=True, exist_ok=True)

    mesh = bpy.data.meshes.new(object_name + "Mesh")
    mesh.from_pydata(VERTICES, (), faces)
    mesh.update(calc_edges=True)

    cube = bpy.data.objects.new(object_name, mesh)
    bpy.context.collection.objects.link(cube)
    cube.data.materials.append(create_material(object_name + "Material", color))

    bpy.context.view_layer.objects.active = cube
    cube.select_set(True)

    blend_path = output_directory / (object_name + ".blend")
    x_path = output_directory / (object_name + ".X")
    bpy.ops.wm.save_as_mainfile(filepath=str(blend_path))

    bpy.ops.export_scene.directx_x(
        filepath=str(x_path),
        use_selection=True,
        use_mesh_modifiers=True,
        global_scale=1.0,
        axis_forward="Z",
        axis_up="Y",
        export_normals=True,
        export_uvs=True,
        export_materials=True,
        export_textures=True,
        export_armature=False,
        export_weights=False,
        export_animation=False,
        unweld_on_export=True,
        export_format="TEXT_X",
        triangulate=True,
    )


def main():
    if not hasattr(bpy.ops.export_scene, "directx_x"):
        bpy.ops.preferences.addon_enable(module="bl_ext.blender_org.io_directx_x")

    clockwise_faces = tuple(tuple(reversed(face)) for face in COUNTERCLOCKWISE_FACES)
    export_cube(
        "clockwise",
        "clockwise_cube",
        clockwise_faces,
        (0.8, 0.12, 0.04, 1.0),
    )
    export_cube(
        "counterclockwise",
        "counterclockwise_cube",
        COUNTERCLOCKWISE_FACES,
        (0.04, 0.2, 0.8, 1.0),
    )


if __name__ == "__main__":
    main()
