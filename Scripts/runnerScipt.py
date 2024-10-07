import os
import subprocess

def run_game_engine_with_inputs():
    # Define the input folder and the path to the GameEngine executable
    input_folder = "../../Resources/inputs/"
    game_engine_exe = "GameEngine.exe"
    
    # Get all files in the input folder
    input_files = [f for f in os.listdir(input_folder) if os.path.isfile(os.path.join(input_folder, f))]

    # Run GameEngine.exe with each file as an argument
    for input_file in input_files:
        input_path = os.path.join(input_folder, input_file)
        print(input_path)
        # Run GameEngine.exe with the input file as an argument
        print(f"Running {game_engine_exe} with {input_file}")
        process = subprocess.run([game_engine_exe, input_path], check=True)
        
        # Wait for the process to complete before moving to the next file
        if process.returncode == 0:
            print(f"Finished running {input_file}")
        else:
            print(f"Error running {input_file}")

if __name__ == "__main__":
    run_game_engine_with_inputs()
