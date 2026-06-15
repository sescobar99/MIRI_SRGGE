# Setup and Execution

## Install

1. Extract the project.

2. Extract `dependencies.zip` in the project root directory.
```bash
unzip dependencies.zip
```
3. Extract `models.zip` into the `models/` directory.
```bash
unzip models.zip -d models
```
4. Navigate to the build directory and generate the build files:
```bash
cd build
cmake ..
```

### Optional Configuration

Before compiling, the following values can be modified in `Constants.h`:

```cpp
FACE_BUDGET
HYSTERESIS_FRAMES
```

* `FACE_BUDGET`: triangle budget used by the time-critical rendering algorithm.
* `HYSTERESIS_FRAMES`: number of frames used for LOD hysteresis.

## Compile and Run

Compile and execute the project with:

```bash
make -j && ./SRGGE ../map.txt
```

### Available Maps

The default map is:

```text
map.txt
```

Other available maps:

```text
map_dragon.txt
map_lucy.txt
```
These 2 maps load larger models in order to obtain a low fps count while rendering the base model

### Lucy

- The Lucy model is **not included** in `models.zip` due to file size limitations.
- To run `map_lucy.txt`, manually add the Lucy model files to the `models/` directory before execution.

Example:

```bash
make -j && ./SRGGE ../map_lucy.txt
```

### Running with Normal Clustering

- By default the project uses vertex clustering with average as representative.
- To execute the project using normal clustering implementation (thin feature preservation) use:

```bash
make -j && ./SRGGE ../map.txt normalClustering
```





# User Manual

## Starting the Application

Compile and start the application as stated in the previous section. e.g.
```bash
make -j && ./SRGGE ../map.txt
```

Reminder:

- To use the normal clustering (thin feature preservation):

```bash
./SRGGE ../map.txt normalClustering
```

## Navigation

### Camera Controls

| Key   | Action                         |
| ----- | ------------------------------ |
| W     | Move forward                   |
| A     | Move left                      |
| S     | Move backward                  |
| D     | Move right                     |
| Shift | Increase movement speed        |
| F1    | Enable/disable camera & movement |
| F5    | Toggle fullscreen              |
| Esc   | Exit application               |

## LOD Controls

The application supports five Levels of Detail (LOD):

| Key | LOD Level                       |
| --- | ------------------------------- |
| 0   | LOD 0 (Original model - finest) |
| 1   | LOD 1                           |
| 2   | LOD 2                           |
| 3   | LOD 3                           |
| 4   | LOD 4 (Coarsest model)          |

By default, all models start using the coarsest available LOD.

### Time-Critical Rendering

To enable dynamic LOD selection using the time-critical rendering algorithm, press:

```text
5
```


The algorithm automatically selects the LOD of each object according to the available triangle budget.

### Triangle Budget Controls

The face budget can be modified at runtime using:

| Key      | Action               |
| -------- | -------------------- |
| Numpad + | Increase face budget |
| Numpad - | Decrease face budget |
| I        | Increase face budget |
| K        | Decrease face budget |

Higher budgets allow more detailed models to be rendered.

### LOD Color Visualization

To visualize the LOD level of each model using solid colors, press:

```text
C
```



| LOD   | Color  |
| ----- | ------ |
| LOD 0 | White  |
| LOD 1 | Green  |
| LOD 2 | Yellow |
| LOD 3 | Orange |
| LOD 4 | Red    |

This mode is useful for debugging the time-critical rendering algorithm.

## FPS Display

Press:

```text
F
```

to enable or disable FPS statistics.

FPS display is disabled by default.

## Configuration Parameters

Several parameters can be modified in `Constants.h` before compilation.

### Face Budget

```cpp
FACE_BUDGET
```

Initial triangle budget used by the time-critical rendering algorithm.

### Hysteresis Frames

```cpp
HYSTERESIS_FRAMES
```

Number of frames required before an LOD transition is accepted.

Increasing this value reduces visible popping at the cost of slower LOD adaptation.