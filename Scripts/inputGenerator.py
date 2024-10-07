import json

GENERATED_PATH = "../Resources/inputs"

def generate_entity_json(num_models, render_method, model):
    # Define the base structure
    entities = []
    
    # Add models with transform components
    for i in range(num_models):
        model_entity = {
            "Components": [
                {
                    "typename": "Engine::Components::Transform",
                    "position": {
                        "x": i * 2,  # Increment x position for each model
                        "y": 0,
                        "z": 0
                    },
                    "scale": {
                        "x": 1,
                        "y": 1,
                        "z": 1
                    }
                },
                {
                    "typename": "Engine::Components::Model",
                    "path": model
                }
            ]
        }
        entities.append(model_entity)

    # Add camera entity
    camera_entity = {
        "Components": [
            {
                "typename": "Engine::Components::Transform",
                "position": {
                    "x": 0,
                    "y": 0,
                    "z": -5
                }
            },
            {
                "typename": "Engine::Components::Tag",
                "tag": "MainCamera"
            }
        ]
    }
    entities.append(camera_entity)

    # Add rendering method entity
    render_entity = {
        "Components": [
            {
                "typename": "Engine::Components::Tag",
                "tag": render_method
            }
        ]
    }
    entities.append(render_entity)
    
    model_name = model.split("/")[-1].split(".")[0]

    # Generate filename for StatsConfiguration
    stats_filename = f"../../Resources/Stats/stats_{num_models}_{render_method}_{model_name}.txt"

    # Add StatsConfiguration entity
    stats_entity = {
        "Components": [
            {
                "typename": "Engine::Components::FileLocation",
                "path": stats_filename
            },
            {
                "typename": "Engine::Components::Tag",
                "tag": "StatsConfiguration"
            }
        ]
    }
    entities.append(stats_entity)

    # Construct the final JSON data
    json_data = {
        "Entities": entities
    }

    # Write JSON data to file
    output_filename = f"{GENERATED_PATH}/entities_{num_models}_{render_method}_{model_name}.json"
    with open(output_filename, "w") as outfile:
        json.dump(json_data, outfile, indent=4)
    
    print(f"Generated JSON file: {output_filename}")

# Usage example

rendering_methods = ["DirectX", "OpenGL", "Vulkan"]
models = ["../../Resources/cube/cube.obj", "../../Resources/bunny/bunny.obj", "../../Resources/cup.obj"]
nums = [0, 5, 10, 15, 20, 25]
for m in models:
    for r in rendering_methods:
        for n in nums:
            generate_entity_json(num_models=n, render_method=r, model=m)

print("Ready")