6
÷

D
AddV2
x"T
y"T
z"T"
Ttype:
2	
x
Assign
ref"T

value"T

output_ref"T"	
Ttype"
validate_shapebool("
use_lockingbool(
8
Const
output"dtype"
valuetensor"
dtypetype
.
Identity

input"T
output"T"	
Ttype
e
MergeV2Checkpoints
checkpoint_prefixes
destination_prefix"
delete_old_dirsbool(
?
Mul
x"T
y"T
z"T"
Ttype:
2	

NoOp
M
Pack
values"T*N
output"T"
Nint(0"	
Ttype"
axisint 
C
Placeholder
output"dtype"
dtypetype"
shapeshape:
X
PlaceholderWithDefault
input"dtype
output"dtype"
dtypetype"
shapeshape
o
	RestoreV2

prefix
tensor_names
shape_and_slices
tensors2dtypes"
dtypes
list(type)(0
l
SaveV2

prefix
tensor_names
shape_and_slices
tensors2dtypes"
dtypes
list(type)(0
?
Select
	condition

t"T
e"T
output"T"	
Ttype
H
ShardedFilename
basename	
shard

num_shards
filename
@
StaticRegexFullMatch	
input

output
"
patternstring
N

StringJoin
inputs*N

output"
Nint(0"
	separatorstring 
<
Sub
x"T
y"T
z"T"
Ttype:
2	
s

VariableV2
ref"dtype"
shapeshape"
dtypetype"
	containerstring "
shared_namestring "serve*2.9.02unknownÜ&
q
input1Placeholder*+
_output_shapes
:˙˙˙˙˙˙˙˙˙*
dtype0* 
shape:˙˙˙˙˙˙˙˙˙
q
input2Placeholder*+
_output_shapes
:˙˙˙˙˙˙˙˙˙*
dtype0* 
shape:˙˙˙˙˙˙˙˙˙

var1/Initializer/ConstConst*
_class
	loc:@var1*"
_output_shapes
:*
dtype0*!
valueB*  PA

var1
VariableV2*
_class
	loc:@var1*"
_output_shapes
:*
	container *
dtype0*
shape:*
shared_name 
˘
var1/AssignAssignvar1var1/Initializer/Const*
T0*
_class
	loc:@var1*"
_output_shapes
:*
use_locking(*
validate_shape(
a
	var1/readIdentityvar1*
T0*
_class
	loc:@var1*"
_output_shapes
:

var2/Initializer/ConstConst*
_class
	loc:@var2*"
_output_shapes
:*
dtype0*!
valueB*  B

var2
VariableV2*
_class
	loc:@var2*"
_output_shapes
:*
	container *
dtype0*
shape:*
shared_name 
˘
var2/AssignAssignvar2var2/Initializer/Const*
T0*
_class
	loc:@var2*"
_output_shapes
:*
use_locking(*
validate_shape(
a
	var2/readIdentityvar2*
T0*
_class
	loc:@var2*"
_output_shapes
:
S
mulMulinput1	var1/read*
T0*+
_output_shapes
:˙˙˙˙˙˙˙˙˙
U
mul_1Mulinput2	var2/read*
T0*+
_output_shapes
:˙˙˙˙˙˙˙˙˙
N
addAddV2mulmul_1*
T0*+
_output_shapes
:˙˙˙˙˙˙˙˙˙
J
sub/yConst*
_output_shapes
: *
dtype0*
valueB
 *  4B
L
subSubaddsub/y*
T0*+
_output_shapes
:˙˙˙˙˙˙˙˙˙
M
outputIdentitysub*
T0*+
_output_shapes
:˙˙˙˙˙˙˙˙˙
(
initNoOp^var1/Assign^var2/Assign
Y
save/filename/inputConst*
_output_shapes
: *
dtype0*
valueB Bmodel
n
save/filenamePlaceholderWithDefaultsave/filename/input*
_output_shapes
: *
dtype0*
shape: 
e

save/ConstPlaceholderWithDefaultsave/filename*
_output_shapes
: *
dtype0*
shape: 
{
save/StaticRegexFullMatchStaticRegexFullMatch
save/Const"/device:CPU:**
_output_shapes
: *
pattern
^s3://.*
a
save/Const_1Const"/device:CPU:**
_output_shapes
: *
dtype0*
valueB B.part
f
save/Const_2Const"/device:CPU:**
_output_shapes
: *
dtype0*
valueB B
_temp/part
|
save/SelectSelectsave/StaticRegexFullMatchsave/Const_1save/Const_2"/device:CPU:**
T0*
_output_shapes
: 
w
save/StringJoin
StringJoin
save/Constsave/Select"/device:CPU:**
N*
_output_shapes
: *
	separator 
Q
save/num_shardsConst*
_output_shapes
: *
dtype0*
value	B :
k
save/ShardedFilename/shardConst"/device:CPU:0*
_output_shapes
: *
dtype0*
value	B : 

save/ShardedFilenameShardedFilenamesave/StringJoinsave/ShardedFilename/shardsave/num_shards"/device:CPU:0*
_output_shapes
: 
z
save/SaveV2/tensor_namesConst"/device:CPU:0*
_output_shapes
:*
dtype0*
valueBBvar1Bvar2
v
save/SaveV2/shape_and_slicesConst"/device:CPU:0*
_output_shapes
:*
dtype0*
valueBB B 

save/SaveV2SaveV2save/ShardedFilenamesave/SaveV2/tensor_namessave/SaveV2/shape_and_slicesvar1var2"/device:CPU:0*
dtypes
2
 
save/control_dependencyIdentitysave/ShardedFilename^save/SaveV2"/device:CPU:0*
T0*'
_class
loc:@save/ShardedFilename*
_output_shapes
: 
Ź
+save/MergeV2Checkpoints/checkpoint_prefixesPacksave/ShardedFilename^save/control_dependency"/device:CPU:0*
N*
T0*
_output_shapes
:*

axis 

save/MergeV2CheckpointsMergeV2Checkpoints+save/MergeV2Checkpoints/checkpoint_prefixes
save/Const"/device:CPU:0*
delete_old_dirs(

save/IdentityIdentity
save/Const^save/MergeV2Checkpoints^save/control_dependency"/device:CPU:0*
T0*
_output_shapes
: 
}
save/RestoreV2/tensor_namesConst"/device:CPU:0*
_output_shapes
:*
dtype0*
valueBBvar1Bvar2
y
save/RestoreV2/shape_and_slicesConst"/device:CPU:0*
_output_shapes
:*
dtype0*
valueBB B 
¤
save/RestoreV2	RestoreV2
save/Constsave/RestoreV2/tensor_namessave/RestoreV2/shape_and_slices"/device:CPU:0*
_output_shapes

::*
dtypes
2

save/AssignAssignvar1save/RestoreV2*
T0*
_class
	loc:@var1*"
_output_shapes
:*
use_locking(*
validate_shape(

save/Assign_1Assignvar2save/RestoreV2:1*
T0*
_class
	loc:@var2*"
_output_shapes
:*
use_locking(*
validate_shape(
8
save/restore_shardNoOp^save/Assign^save/Assign_1
-
save/restore_allNoOp^save/restore_shard"ś<
save/Const:0save/Identity:0save/restore_all (5 @F8"
trainable_variables
>
var1:0var1/Assignvar1/read:02var1/Initializer/Const:08
>
var2:0var2/Assignvar2/read:02var2/Initializer/Const:08"
	variables
>
var1:0var1/Assignvar1/read:02var1/Initializer/Const:08
>
var2:0var2/Assignvar2/read:02var2/Initializer/Const:08*˛
serving_default
-
input1#
input1:0˙˙˙˙˙˙˙˙˙
-
input2#
input2:0˙˙˙˙˙˙˙˙˙-
output#
output:0˙˙˙˙˙˙˙˙˙serving_default