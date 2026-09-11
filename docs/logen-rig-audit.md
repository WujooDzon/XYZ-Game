# Logen Rig Audit — Stage 01C

The audit covers the supplied clean alpha-isolated components in Assets/Characters/Logen/ and the canonical reference/master. PNG pixels were not changed.

## Active composition

| Component | Asset | Decision |
| --- | --- | --- |
| head | Logen_rig_head_mask_hood.png | use, attached to torso |
| right side arm | Logen_rig_right_empty_sleeve.png | use; intentionally no forearm or hand |
| torso | Logen_rig_torso_upper.png | use as the upper body base |
| waist/front accessory | Logen_rig_waist_belt_front.png | use; contains the belt, diagonal strap, potions, and red hanging cloth |
| left upper arm | Logen_rig_left_upper_arm.png | use |
| left forearm/hand | Logen_rig_left_forearm_hand.png | use |
| back cloak | Logen_rig_cloak_back_full.png | use behind the body |
| front cloak left/right | Logen_rig_cloak_front_left.png, Logen_rig_cloak_front_right.png | use as side strips behind the legs; their z-order must not hide the boots |
| red cloth | Logen_rig_red_cloth_front.png | exclude; duplicated by the combined waist/front accessory |
| thighs | Logen_rig_left_thigh.png, Logen_rig_right_thigh.png | use; separate articulated leg segments |
| shins | Logen_rig_left_shin.png, Logen_rig_right_shin.png | use; separate articulated leg segments |
| boots | Logen_rig_left_boot.png, Logen_rig_right_boot.png | use; each has an explicit ground-contact anchor |

The resulting rig has one textureless pelvis root plus 15 active art nodes. The standalone red cloth is not layered on top of the waist/front component, which prevents duplicated cloth, belt, and potion silhouettes. The thighs, shins, and boots are the only leg pieces, so no trouser or boot duplicates are introduced.

## Reference and legacy assets

Logen_Master_Right_v1.png is the canonical right-facing full-character reference. It is not part of normal gameplay rendering; F4 draws it as a 35% alpha calibration overlay at the same target height and fixed ground anchor.

Logen_idle_right_01.png–04.png and Logen_walk_right_01.png–08.png are legacy full-frame references. They remain unchanged for provenance, but the active scene does not load or render them. Stage 01C uses only the hierarchical rig and JSON animation data.

## Calibration observations

- All node positions, animation offsets, pivots, and root_to_ground use one source-rig coordinate space.
- target_height: 188 is normalized from the measured neutral assembled bounds; global_scale: 1 is only a post-normalization multiplier.
- The root is grounded from fixed rig metadata, not from animated bounds.
- The right sleeve remains empty in neutral and every walk pose.
- The upper body is kept restrained while the legs alternate contact/down/passing/up poses.
