import json

def remove_parent_components(file_path):
    try:
        # Open and load the JSON file
        with open(file_path, 'r') as file:
            data = json.load(file)

        # Counter for removed components
        removed_count = 0

        # Iterate through entities and filter components
        if 'data' in data and 'entities' in data['data']:
            for entity in data['data']['entities']:
                if 'components' in entity:
                    original_count = len(entity['components'])
                    
                    # Filter out components where type is "Components::Parent"
                    entity['components'] = [
                        comp for comp in entity['components'] 
                        if comp.get('type') != 'Components::Parent'
                    ]
                    
                    # Update count
                    removed_count += (original_count - len(entity['components']))

        # Save the modified JSON back to the file (or a new one)
        output_file = 'LevelOne_Cleaned.json'
        with open(output_file, 'w') as file:
            json.dump(data, file, indent=4)

        print(f"Successfully removed {removed_count} 'Components::Parent' entries.")
        print(f"Saved cleaned file to: {output_file}")

    except Exception as e:
        print(f"An error occurred: {e}")

# Run the function
if __name__ == "__main__":
    remove_parent_components('LevelOne.json')