import json
import copy

GENERATED_PATH = "../Configs"
STATS_PATH = "../Statistics"

EXPERIMENT_TIME = 20

ORIGINAL_CONFIG = {
    "Prefabs": [
	    {
			"Name": "Cube",
            "Components": [
                {
                    "typename": "Engine::Components::Model",
                    "path": "../Models/cube.obj"
                },
	            {
                    "typename": "Engine::Components::Transform",
                    "position": {
                        "x": 0,
                        "y": 0,
                        "z": 0
                    },
					"scale": {
						"x": 0.3,
						"y": 0.3,
						"z": 0.3
					}
                }
            ]
        },
		{
			"Name": "Bunny",
            "Components": [
                {
                    "typename": "Engine::Components::Model",
                    "path": "../Models/bunny.obj"
                },
	            {
                    "typename": "Engine::Components::Transform",
                    "position": {
                        "x": 0,
                        "y": 0,
                        "z": 0
                    },
					"scale": {
						"x": 0.2,
						"y": 0.2,
						"z": 0.2
					}
                }
            ]
        }
	],
    "Entities": [
        {
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
    ],
    "Systems": [
		{
			"typename": "Engine::Systems::InputSystem"
		}
	]
}

def create_dynamic_experiment(prefab_name, prefab_count):
    return {
		"typename": "Engine::Systems::Experiment1System",
		"prefab": prefab_name,
		"experimentTime": 20,
		"prefabCount": prefab_count,
		"rotationSpeed": 1,
		"radiuses": [1, 1.5, 2.5, 3.5],
		"cameraMaxDistance": 5,
		"cameraSpeed": 2.0
	}
    
def create_static_experiment(prefab_name, prefab_count):
    return {
		"typename": "Engine::Systems::Experiment2System",
		"prefab": prefab_name,
		"experimentTime": 20,
		"prefabCount": prefab_count,
		"distanceDelta": 0.7,
		"elementsPerRow": 10
	}
    
EXPERIMENTS = [create_dynamic_experiment, create_static_experiment]
    
def create_stats_system(renderer, output_file):
    return {
		"typename": "Engine::Systems::StatsSystem",
		"outputFile": output_file,
		"renderer": renderer
	}
    
def create_rendering_system(renderer):
    return {
		"typename": "Engine::Systems::RenderingSystem",
		"renderer": renderer
    }

def create_config(renderer, prefab_name, prefab_count, experiment):
    config = copy.deepcopy(ORIGINAL_CONFIG)

    config["Systems"].append(EXPERIMENTS[experiment](prefab_name, prefab_count))
    stats_filename = f"{STATS_PATH}/stats_{renderer}_{prefab_name}_{prefab_count}_{experiment + 1}.txt"
    config["Systems"].append(create_stats_system(renderer, stats_filename))
    config["Systems"].append(create_rendering_system(renderer))
    
    # Write JSON data to file
    output_filename = f"{GENERATED_PATH}/config_{renderer}_{prefab_name}_{prefab_count}_{experiment + 1}.json"
    with open(output_filename, "w") as outfile:
        json.dump(config, outfile, indent=4)
    
    print(f"Generated JSON file: {output_filename}")

# Usage example

rendering_methods = ["DirectX", "OpenGL", "Vulkan"]
prefabs = ["Cube", "Bunny"]
prefabs_count = {0: [25, 50, 100, 200, 300, 400, 500], 1: [100, 200, 400, 800, 1200, 1600, 2000]}
configs_amount = 0
for renderer in rendering_methods:
    for prefab_name in prefabs:
        for experiment in prefabs_count:
            for amount in prefabs_count[experiment]:
                create_config(renderer, prefab_name, amount, experiment)
                configs_amount += 1

print(f"Generated {configs_amount} configs")