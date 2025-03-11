import os
import subprocess
import json

def run_game_engine_with_inputs():
    # Define the input folder and the path to the GameEngine executable
    input_folder = "../../Configs/"
    game_engine_exe = "GameEngine.exe"
    
    for i in range(5):
        warmup_path = os.path.join(input_folder, "config_DirectX_Teapot_2000_2.json")
        process = subprocess.run([game_engine_exe, warmup_path], check=True)
        print("Runned warmup experiment:", i+1)
    
    # Get all files in the input folder
    input_files = ["config_DirectX_Bunny_2000_2.json", "config_OpenGL_Bunny_2000_2.json", "config_Vulkan_Bunny_2000_2.json",
                    "config_DirectX_Cube_2000_2.json", "config_OpenGL_Cube_2000_2.json", "config_Vulkan_Cube_2000_2.json",
                    "config_DirectX_Teapot_2000_2.json", "config_OpenGL_Teapot_2000_2.json", "config_Vulkan_Teapot_2000_2.json"]
    repeats = 20
    # Run GameEngine.exe with each file as an argument
    for input_file in input_files:
        input_path = os.path.join(input_folder, input_file)
        print(input_path)
        
        with open(input_path) as config_file:
            config = json.loads(config_file.read())
        output_path = os.path.join(input_folder, config["Systems"][2]["outputFile"])
        
        print(output_path[:-4])
        # Run GameEngine.exe with the input file as an argument
        print(f"Running {game_engine_exe} with {input_file}")
        final_output = ""
        for i in range(repeats):
            process = subprocess.run([game_engine_exe, input_path], check=True)
        
            # Wait for the process to complete before moving to the next file
            if process.returncode == 0:
                print(f"Finished running {input_file}")
            else:
                print(f"Error running {input_file}")
                continue
            
            with open(output_path) as output_file:
                final_output += output_file.read() + "\n\n"
            
        with open(f"{output_path[:-4]}_all.txt", "w") as final_file:
            final_file.write(final_output)

if __name__ == "__main__":
    run_game_engine_with_inputs()
