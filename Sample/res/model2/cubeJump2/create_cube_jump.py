import bpy
from pathlib import Path


output_directory = Path(__file__).resolve().parent
blend_path = output_directory / "cube_jump_blender_5_1_2.blend"
x_path = output_directory / "cube_jump_blender_5_1_2.x"

bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)

bpy.ops.mesh.primitive_cube_add(location=(0.0, 0.0, 0.0))
cube = bpy.context.active_object
cube.name = "JumpCube"
cube.scale = (0.5, 0.5, 0.5)
bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)

material = bpy.data.materials.new(name="JumpCubeBlue")
material.diffuse_color = (0.12, 0.42, 0.95, 1.0)
material.specular_intensity = 0.35
material.roughness = 0.4
cube.data.materials.append(material)

bpy.ops.object.armature_add(enter_editmode=True, location=(0.0, 0.0, 0.0))
armature = bpy.context.active_object
armature.name = "JumpRig"
bone = armature.data.edit_bones[0]
bone.name = "JumpRoot"
bone.head = (0.0, 0.0, 0.0)
bone.tail = (0.0, 0.0, 1.0)
bpy.ops.object.mode_set(mode="OBJECT")

vertex_group = cube.vertex_groups.new(name="JumpRoot")
vertex_group.add(list(range(len(cube.data.vertices))), 1.0, "REPLACE")

modifier = cube.modifiers.new(name="JumpRig", type="ARMATURE")
modifier.object = armature
cube.parent = armature

bpy.context.scene.frame_start = 1
bpy.context.scene.frame_end = 31
bpy.context.scene.render.fps = 30

pose_bone = armature.pose.bones["JumpRoot"]
pose_bone.location = (0.0, 0.5, 0.0)
pose_bone.keyframe_insert(data_path="location", frame=1)
pose_bone.location = (0.0, 2.0, 0.0)
pose_bone.keyframe_insert(data_path="location", frame=16)
pose_bone.location = (0.0, 0.5, 0.0)
pose_bone.keyframe_insert(data_path="location", frame=31)

if armature.animation_data is not None and armature.animation_data.action is not None:
    armature.animation_data.action.name = "Jump"

bpy.context.scene.frame_set(1)
bpy.ops.wm.save_as_mainfile(filepath=str(blend_path))

bpy.ops.object.select_all(action="DESELECT")
cube.select_set(True)
armature.select_set(True)
bpy.context.view_layer.objects.active = armature

bpy.ops.export_scene.directx_x(
    filepath=str(x_path),
    check_existing=False,
    use_selection=True,
    use_mesh_modifiers=True,
    global_scale=1.0,
    axis_forward="Z",
    axis_up="Y",
    export_normals=True,
    export_uvs=True,
    export_materials=True,
    export_textures=False,
    export_armature=True,
    export_weights=True,
    export_animation=True,
    anim_key_format="TRS",
    unweld_on_export=True,
    export_format="TEXT_X",
    anim_fps=30.0,
    anim_frame_start=1,
    anim_frame_end=31,
    triangulate=True,
)

print(f"BLEND_OUTPUT={blend_path}")
print(f"X_OUTPUT={x_path}")
