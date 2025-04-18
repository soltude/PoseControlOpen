

## Creating bones and constraints for PhAt Tissue breast and glutes


Core bones go through the center of the body part, which may have Spoke bones as children, which have Point bone children that should terminate below the skin. Sphere colliders are created on each bone, with collision disabled between them (the colliders all overlap). Point colliders should be sized and placed such that the collider approximates the curvature of the body part. Parent bodies should always have more mass then their children for best results.

Constraints are created for each parent-child relationship, then Spoke-Spoke and Point-Point constraints are created based on a naive n-closest-neighbor algorithm. You can also add constraints between other bodies and their closest points/spokes (This is useful for constraining the base breast point bones to the clavicle and spine.) Parent-child and Point-Bone joints should have linear and angular limits set; Point-Point constraints should only need linear limits (angular is free.) Linear and angular drives are created and the drive strength is set proportional to the mass of the child body. 

1. In you physics asset, create sphere colliders on all Core, Spoke, and Point bones. 
2. Copy  `DA_PcActorDataAsset` to your project
   * `TargetSkeletalMesh`: your imported SkeletalMesh
   * `TargetPhysicsAsset`: A physics asset for that SKM 
   * `FPhatConstraintOptions`: Struct with all settings for generating constraints
     * for each of GluteCores, etc., make sure the `PointPatternString` regex will match your bodies
3. Using the `EUW_Phat` Editor Widget, load you PcActorDataAsset and click `Add Point Constraints`
   * Alternatively, call `AddPhatConstraints`, but you will need a pointer to a `USkeletalMeshComponenent` on an Actor in world, either the EditorWorld or the PhysicsAssetEditor.

## Generating Physics Assets (simple, no skeletal morphs.)

*If using skeletal morphs or pose drivers, complete the following section first.*

1. Copy  `DA_PcActorDataAsset` to your project
   * `TargetSkeletalMesh`: your imported SkeletalMesh 
   * `TargetPhysicsAsset`: You can copy `PA_Genesis9` into your folder for a basic constraint and collision set up. Make sure in the PhysicsAsset to set your own character as the preview mesh and click "apply to asset".
   * `BodyParamsDataTable`: DataTable where each row contains a bone name and what type of physics body primitive should be created for it. You can use or modify the one in the plugin Content folder. (DataTable is a wrapper around builtin struct `FPhysAssetCreateParams`, the options in the "Tools" panel the PhAt editor.)
   * `TwistNames` - names of twist bones, (explained below)
   * `CapsuleNames` - names of capsule bones (explained below)
   * You can ignore the rest of the values if you're only doing this part. 
2. Call `CreateBodiesFromDataTable`.
   * Creates a physics body for each row in `BodyParamsDataTable`.

3. Call `CopyTwistShapesToParents`.
   * Copies the physics primitives generated for the twist bones to their parent bone in that limb. I haven't found a way to lock the swing axes to uses physics on twist bones without the arms looking broken. Twist JCMs *should* still mostly work I think, but will be driven by overall rotation of the limb.
   * Will use the names from `TwistNames` if set, otherwise the default UE5 skeleton twist bones for upperarm, lowerarm, and thigh.
   * if `bDeleteChildBodies` is true, the bodies for the twist bones will be deleted after they are copied to the parent physics body.

4. Call `AlignCapsuleToBone`.
   * Tries to do a slightly better job aligning capsule primitives to meshes, mainly for finger and toe bones. Will usually need manual adjustment, YMMV if this is helpful.
   * (I'm not sure if there's actually any performance gain vs. just using convex hulls on fingers and toes.)
   * `CapsuleNames` will be created for you if ran the `CreatePhysicsAssetAndMorphedSkm` function, otherwise you can set it manually.
   
5. `FixConstraintScale` will reset the scale vector to identity in any constraint transforms in the TargetPhysicsAsset which may have inadvertently been set. You probably won't need this but if you get any warnings in the logs about constraint scale, this should fix it.

6. (Only if you followed the morphed steps below). 
   * After making any adjustments in the PhAt editor for your new PhAt, you will need to set the preview mesh to your original skeletal mesh and click `Apply To Asset`. It will no longer look correct in the PhAt editor, we'll fix that in game with an AnimBP.
   * After this, you no longer need and can delete the generated SKM, it was just needed to create the physics asset, and won't work as an SKM on its own. You might want it to make any further adjustments on the PhAt, or you can regenerate it. 

## Editor setup for using multiple PhAts on one SKM with morphs and/or posedrivers

1. Copy `ABP_CopyMesh` into your project and give it a unique name. 
   * In the AnimGraph, after the `Modify Curve` node, insert any PoseAsset nodes which you will be using.
1. Copy  `DA_PcActorDataAsset` to your project
   * fill out `CharacterName`, `NewAssetPath`, 
   * `SourceSkeletalMesh`, is your imported SkeletalMesh `SourcePhysicsAsset`.
   * You can copy `PA_Genesis9` into your folder for a basic constraint and collision set up. Make sure in the PhysicsAsset to set your own character as the preview mesh and click "apply to asset". This will be your `SourcePhysicsAsset`.
   * For `CurveMap`, map the name of a PoseAsset or other animation curve to the float (0 to 1) value for that curve. (for DTH users, this is anything starting with "pas...")
   * `MorphTargetMap`: Same as `CurveMap`, but for skeletal morph targets. (It might actually work just putting them all in `CurveMap`, haven't tried it.)
1. Call `CreatePhysicsAssetAndMorphedSkm`. 
   * Duplicates the PhysicsAsset
   * Create a new SkeletalMesh Asset by copying the morphed SkeletalMeshComponent pose to a dynamic mesh, then calling `UE::AssetUtils::CreateSkeletalMeshAsset`.
1. You should see two new files in `NewAssetPath`, one called `PA_<YourCharcater>` and one `SKM_<YourCharcater>`, and these will be set in your PcActorDataAsset as the Target PhAt and SKM.
1. Follow the `Generating Physics Assets` steps above. 
   * You shouldn't need to set Target PhAt or SKM.

## Making it work in game. 

To make everything come together in the game, we need our SkeletalMesh component to apply the curves and/or morph targets, set it to use our new physics asset, and then set our physics asset (which is applied to the base SKM) so the constraints' reference frame position and orientation are set to match the modified skeleton pose. 

1. I'm not sure if this can be done in a post process animBP, so I've been using my modifed ABP_CopyMesh as a linked AnimGraph in my main ABP for the character. The important thing seems to be that your posedrivers which modify the skeleton run before the physics tick. 
2. In your characterBP or wherever you control your actor, set the PhysicsAsset override to your new PhAt.
3. 