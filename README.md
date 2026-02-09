# SoftRef
Bring Unreal's soft references to Godot. 

<img width="324" height="249" alt="obraz" src="https://github.com/user-attachments/assets/a770d458-2006-4cb5-942f-803c334642d6" />

## What's a SoftRef?
***SoftRef*** holds a reference to a resource without forcing it to be loaded immediately. It allows for both sync and async loading depending on your needs. 

If you’re coming from Unreal Engine, it is similar to a *Soft Object Reference* (`TSoftObjectPtr`) for Godot's Resources.

## Features:
- On-demand loading (both sync and async)
- Explicit `LoadState` enum
- `loading_finished` signal for clean async workflows
- Centralized, deduplicated threaded loading
- Check load progress with `get_progress()` - useful for loading bars
- Safe lifetime management - resources are only released when you call `reset()`
- Works with both resource paths and UIDs

## Why use SoftRef?
Godot already provides numerous tools to load resources:
- `load()`/`preload()` for hard references
- `ResourceLoader` for manual and threaded loading
- `ResourcePreloader` for scene-based setups
- `InstancePlaceholder` for defered node loads inside an already existing scene

But there's no single abstraction for:
- holding a reference without loading
- tracking load state explicitly (using `ResourceLoader.load_threaded_get_status()` can be verbose and not intuitive for beginers)
- sharing async loads across multiple references,
- reacting cleanly when loading finishes.

**SoftRef** fills this gap, while also providing a familiar concept for developers coming from Unreal Engine.

## Use cases
- Dynamic asset loading (UI, tools, runtime content)
- Reducing startup time and memory usage
- Replacing ad-hoc `ResourceLoader` boilerplate
- When `preload` is too eager and too permanent

### Example
```gdscript
@export_file_path var _file_path: String # Stores a file path, e.g. 'res://icon.svg'
@export_file var _file_uid: String # Stores a file UID, e.g. 'uid://bxrbpxq663ob7'

func _ready() -> void:
	# Sync load
	var soft := SoftRef.new()
	soft.path = _file_path # Use @export_file_path if you want to store path.
	print("Soft before load: ", soft)
	# <SoftRef#-9223372001354185274>(Empty - Resource: null)

	soft.load_sync() # Blocks current thread until loading finishes
	print("Soft after load_sync(): ", soft)
	# <SoftRef#-9223372001354185274>(Loaded - Resource: (res://icon.svg):<CompressedTexture2D#-9223372001253521965>)

	soft.reset()
	print("Soft after reset(): ", soft)
	# <SoftRef#-9223372001354185274>(Empty - Resource: null)
	 
	# Async load
	var soft2 := SoftRef.new()
	soft2.uid = _file_uid # Assign UID when using @export_file.
	print("Soft2 before load: ", soft2)
	# <SoftRef#-9223372001119304237>(Empty - Resource: null)

	soft2.load_async()
	print("Soft2 load_async call: ", soft2)
	# <SoftRef#-9223372001119304237>(Pending - Resource: null)

	SoftRefLoader.request_load(soft2) # Will raise a warning - resource is already being loaded.

	await soft2.loading_finished
	if soft2.is_loaded():
		print("Soft2 after loading_finished signal: ", soft2)
		# <SoftRef#-9223372001119304237>(Loaded - Resource: (res://icon.svg):<CompressedTexture2D#-9223372000985086507>)

	# Async load using SoftRefLoader
	var soft3: SoftRef = SoftRefLoader.request_load_from_path(_file_path)
	await soft3.loading_finished
	if soft3.get_load_state() == SoftRef.LoadState.LOADED:
		print("Loaded asset: ", soft3.get())
		# Loaded asset: (res://icon.svg):<CompressedTexture2D#-9223372000985086507>
```

## Documentation
SoftRef is fully documented and integrated with Godot's built-in documentation. All classes, methods, signals, and enums are available directly inside the editor.
<img width="1103" height="672" alt="obraz" src="https://github.com/user-attachments/assets/097fa188-bfe8-485c-a300-d1ece65147fe" />


## Crash course for Unreal Developers

### Referencing an asset without loading it
Unreal:
```cpp
UPROPERTY(EditAnywhere)
TSoftObjectPtr<UTexture2D> Icon;
```

Godot SoftRef:
```gdscript
@export_file_path("Texture2D") var icon_path: String

var icon_soft := SoftRef.new()
icon_soft.path = icon_path
```

### Loading on demand
Unreal:
```cpp
UTexture2D* Texture = Icon.LoadSynchronous();
```

Godot SoftRef:
```gdscript
icon_soft.load_sync()
var texture := icon.get()
```

### Asynchronous loading
Unreal:
```cpp
Icon.LoadAsync(FStreamableDelegate::CreateLambda([this]()
{
    UTexture2D* Texture = Icon.Get();
}));
```

Godot SoftRef:
```gdscript
# First way: SoftRef.load_async()
icon_soft.load_async()
await icon_soft.loading_finished
var texture := icon_soft.get()

# Second way: SoftRefLoader.request_load()
SoftRefLoader.request_load(icon_soft)
await icon_soft.loading_finished
var texture := icon_soft.get()

# Third way: SoftRefLoader.request_load_from_path() / SoftRefLoader.request_load_from_uid():
var new_soft_ref := SoftRefLoader.request_load_from_path(icon_path)
await new_soft_ref.loading_finished
var texture := new_soft_ref.get()
```
No delegates and no polling required - just `await` the `loading_finished` signal!

### Checking load state
Unreal:
```cpp
if (Icon.IsNull()) { ... }
if (Icon.IsPending()) { ... }
if (Icon.IsValid()) { ... }
```
Godot SoftRef:
```gdscript
if icon_soft.is_null():
  ...
if icon_soft.is_pending():
  ...
if icon_soft.is_valid():
  ...
if icon_soft.is_loaded():
  ...
if icon_soft.is_failed():
  ...

# Alternatively, use "match":
match icon.get_load_state():
  SoftRef.LoadState.LOADED:
    print("Loaded!") # Safe to use SoftRef.get() here
  SoftRef.LoadState.EMPTY:
    print("No resource assigned.")
  SoftRef.LoadState.NOT_LOADED:
    print("Resource assigned, loading not started yet.")
  SoftRef.LoadState.PENDING:
    print("Currently loading asset.")
  SoftRef.LoadState.FAILED:
    print("Loading failed!")
```

### Multiple soft references to the same asset
In both cases, loading asset from the same path is deduplicated and shares the same load request. No extra actions are required.

### Releasing the resource
In Unreal, the resource may be unloaded by GC if no hard references remain.
In Godot SoftRef, you need to manually call `reset()`:
```gdscript
  icon_soft.reset()
  # Now, icon_soft.get() will return null.
```
