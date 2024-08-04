import subprocess
import os
import re
import tempfile

def clone_repo(repo_url, branch, temp_dir):
    """Clone the repository into a temporary directory for the given branch."""
    subprocess.run(['git', 'clone', '--branch', branch, '--single-branch', repo_url, temp_dir], check=True)

def parse_file(file_path):
    """Parse the file to extract group and map tuples."""
    group_count = 0
    map_count = 0
    tuples = []

    # Updated regex patterns to match lines starting with a tab followed by the macro
    newgroup_pattern = re.compile(r'^\tnewgroup ')
    map_const_pattern = re.compile(r'^\tmap_const\s+([^,]+)')

    with open(file_path, 'r') as file:
        for line in file:
            if newgroup_pattern.match(line):
                group_count += 1
                map_count = 0  # Reset map count when a new group starts
            elif map_const_pattern.match(line):
                map_count += 1
                map_name = map_const_pattern.match(line).group(1).strip()
                tuples.append((group_count, map_count, map_name))

    return tuples

def find_matching_tuples(tuples1, tuples2):
    """Find and return matching tuples based on map names."""
    # Create a dictionary from tuples2 for easy lookup by map name
    tuples2_dict = {map_name: (group_count, map_count) for group_count, map_count, map_name in tuples2}

    matches = []
    for group_count1, map_count1, map_name1 in tuples1:
        if map_name1 in tuples2_dict:
            group_count2, map_count2 = tuples2_dict[map_name1]
            matches.append(((group_count1, map_count1, map_name1), (group_count2, map_count2, map_name1)))

    return matches

def print_matching_tuples(matching_tuples, output_format):
    """Print matching tuples in the specified format."""
    for tuple1, tuple2 in matching_tuples:
        group_count1, map_count1, map_name1 = tuple1
        group_count2, map_count2, map_name2 = tuple2
        
        if output_format == 'unordered_map':
            print(f"{{{{{group_count1}, {map_count1}}}, {{{group_count2}, {map_count2}}}}},  // {map_name1}")
        else:
            print(f"Branch 1: {tuple1} | Branch 2: {tuple2}")

def main(branch1, branch2, output_format='default', repo_url='https://github.com/Rangi42/polishedcrystal.git', file_path='constants/map_constants.asm'):
    with tempfile.TemporaryDirectory() as temp_dir1, tempfile.TemporaryDirectory() as temp_dir2:
        # Clone and checkout files from each branch into separate temporary directories
        clone_repo(repo_url, branch1, temp_dir1)
        file_path1 = os.path.join(temp_dir1, file_path)
        tuples1 = parse_file(file_path1)

        clone_repo(repo_url, branch2, temp_dir2)
        file_path2 = os.path.join(temp_dir2, file_path)
        tuples2 = parse_file(file_path2)

        # Find matching tuples
        matching_tuples = find_matching_tuples(tuples1, tuples2)

        # Print matching tuples in the specified format
        print_matching_tuples(matching_tuples, output_format)

if __name__ == "__main__":
    import sys
    if len(sys.argv) < 3 or len(sys.argv) > 4:
        print("Usage: python map_tuples.py <branch1> <branch2> [unordered_map]")
    else:
        # Determine the output format based on the third argument
        output_format = 'unordered_map' if len(sys.argv) == 4 and sys.argv[3] == 'unordered_map' else 'default'
        main(sys.argv[1], sys.argv[2], output_format)
