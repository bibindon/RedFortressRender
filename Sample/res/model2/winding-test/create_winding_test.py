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


def create_material(name, texture_path):
    material = bpy.data.materials.new(name=name)
    material.diffuse_color = (1.0, 1.0, 1.0, 1.0)
    material["_x_face_color"] = (1.0, 1.0, 1.0, 1.0)
    material["_x_power"] = 500.0
    material["_x_specular"] = (1.0, 1.0, 1.0)
    material.use_nodes = True
    material.node_tree.nodes.clear()

    output_node = material.node_tree.nodes.new("ShaderNodeOutputMaterial")
    shader_node = material.node_tree.nodes.new("ShaderNodeBsdfPrincipled")
    texture_node = material.node_tree.nodes.new("ShaderNodeTexImage")
    texture_image = bpy.data.images.load(str(texture_path), check_existing=True)
    texture_image.filepath = str(texture_path)
    texture_node.image = texture_image
    texture_node.extension = "REPEAT"

    material.node_tree.links.new(texture_node.outputs["Color"], shader_node.inputs["Base Color"])
    material.node_tree.links.new(texture_node.outputs["Alpha"], shader_node.inputs["Alpha"])
    material.node_tree.links.new(shader_node.outputs["BSDF"], output_node.inputs["Surface"])
    return material, texture_image


def export_cube(directory_name, object_name, faces):
    clear_scene()

    output_directory = ROOT_DIRECTORY / directory_name
    output_directory.mkdir(parents=True, exist_ok=True)
    texture_path = output_directory / (object_name + "_texture.png")
    if not texture_path.exists():
        raise RuntimeError(f"Texture file does not exist: {texture_path}")

    mesh = bpy.data.meshes.new(object_name + "Mesh")
    mesh.from_pydata(VERTICES, (), faces)
    mesh.update(calc_edges=True)

    cube = bpy.data.objects.new(object_name, mesh)
    bpy.context.collection.objects.link(cube)
    material, texture_image = create_material(object_name + "Material", texture_path)
    cube.data.materials.append(material)

    bpy.context.view_layer.objects.active = cube
    cube.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.cube_project(cube_size=1.0, correct_aspect=True)
    bpy.ops.object.mode_set(mode="OBJECT")

    blend_path = output_directory / (object_name + ".blend")
    x_path = output_directory / (object_name + ".X")
    bpy.context.preferences.filepaths.save_version = 0
    bpy.ops.wm.save_as_mainfile(filepath=str(blend_path))
    texture_image.filepath = "//" + texture_path.name
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

    export_cube(
        "clockwise",
        "clockwise_cube",
        COUNTERCLOCKWISE_FACES,
    )
    export_cube(
        "counterclockwise",
        "counterclockwise_cube",
        COUNTERCLOCKWISE_FACES,
    )


if __name__ == "__main__":
    main()
