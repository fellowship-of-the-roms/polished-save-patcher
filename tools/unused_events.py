import subprocess
import os
import re
import tempfile

def clone_repo(repo_url, branch, temp_dir):
	"""Clone the repository into a temporary directory for the given branch."""
	subprocess.run(['git', 'clone', '--branch', branch, '--single-branch', repo_url, temp_dir], check=True)

def parse_event_flags(file_path):
	"""Parse the event_flags.asm file to extract unused event indexes."""
	unused_indexes = []
	current_index = 0

	# Updated regex for flexible matching
	const_pattern = re.compile(r'^\s*const\s+.+')
	const_skip_pattern = re.compile(r'^\s*const_skip\s*; unused')
	const_next_pattern = re.compile(r'^\s*const_next\s+\$(.+)')

	with open(file_path, 'r') as file:
		for line in file:
			line = line.strip()
			
			# Handle `const`
			if const_pattern.match(line):
				current_index += 1

			# Handle `const_skip` with `; unused` comment
			elif const_skip_pattern.match(line):
				unused_indexes.append(current_index)
				current_index += 1

			# Handle `const_next`
			elif const_next_pattern.match(line):
				match = const_next_pattern.search(line)
				if match:
					next_index = int(match.group(1), 16)
					# Add all indexes between current_index and next_index
					unused_indexes.extend(range(current_index, next_index))
					current_index = next_index

	return unused_indexes

def format_cpp_array(unused_indexes, line_limit=10):
	"""Format unused indexes into a C++ array with proper tabbing and line limits."""
	cpp_array = "const std::vector<int> unusedEventIndexes = {\n"
	for i in range(0, len(unused_indexes), line_limit):
		cpp_array += "\t" + ", ".join(map(str, unused_indexes[i:i+line_limit])) + ",\n"
	cpp_array = cpp_array.rstrip(",\n") + "\n};"  # Remove trailing comma and close the array
	return cpp_array

def main(branch, repo_url='https://github.com/Rangi42/polishedcrystal.git', file_path='constants/event_flags.asm'):
	with tempfile.TemporaryDirectory() as temp_dir:
		# Clone the repo into a temporary directory
		clone_repo(repo_url, branch, temp_dir)
		file_path_full = os.path.join(temp_dir, file_path)

		# Parse the file for unused indexes
		unused_indexes = parse_event_flags(file_path_full)

		# Generate C++ array output
		cpp_array = format_cpp_array(unused_indexes)
		print("Paste this C++ array into your code:")
		print(cpp_array)

if __name__ == "__main__":
	import sys
	if len(sys.argv) < 2:
		print("Usage: python unused_event_indexes.py <branch>")
	else:
		main(sys.argv[1])
