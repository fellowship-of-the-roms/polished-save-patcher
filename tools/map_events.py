import subprocess
import os
import re
import tempfile

def clone_repo(repo_url, branch, temp_dir):
	"""Clone the repository into a temporary directory for the given branch."""
	subprocess.run(['git', 'clone', '--branch', branch, '--single-branch', repo_url, temp_dir], check=True)

def parse_event_flags(file_path):
	"""Parse the file to extract events starting with '\tconst'."""
	event_pattern = re.compile(r'^\tconst\s+(\w+)')
	events = []

	with open(file_path, 'r') as file:
		for index, line in enumerate(file):
			if event_pattern.match(line) and not line.startswith('\tconst_def'):
				event_name = event_pattern.match(line).group(1)
				events.append((index, event_name))

	return events

def list_events_difference(events1, events2):
	"""List events in branch1 not found in branch2."""
	events1_set = {event for _, event in events1}
	events2_set = {event for _, event in events2}
	diff = events1_set - events2_set
	return [(index, event) for index, event in events1 if event in diff]

def main(branch1, branch2, repo_url='https://github.com/Rangi42/polishedcrystal.git', file_path='constants/event_flags.asm', list_diff=False):
	with tempfile.TemporaryDirectory() as temp_dir1, tempfile.TemporaryDirectory() as temp_dir2:
		# Clone and checkout files from each branch into separate temporary directories
		clone_repo(repo_url, branch1, temp_dir1)
		file_path1 = os.path.join(temp_dir1, file_path)
		events1 = parse_event_flags(file_path1)

		clone_repo(repo_url, branch2, temp_dir2)
		file_path2 = os.path.join(temp_dir2, file_path)
		events2 = parse_event_flags(file_path2)

		if list_diff:
			# List events in branch1 not in branch2
			diff = list_events_difference(events1, events2)
			print(f"Events in {branch1} but not in {branch2}:")
			for index, event in sorted(diff, key=lambda x: x[0]):
				print(f"{index}: {event}")
		else:
			# Print unordered map of events
			print("// Converts a version 7 event flag to a version 8 event flag")
			print("uint16_t mapV7EventFlagToV8(uint16_t v7) {")
			print("\tstd::unordered_map<uint16_t, uint16_t> indexMap = {")
			for index, event in events1:
				print(f"\t\t{{{index}, {index}}},  // {event}")
			print("\t};\n")
			print("\t// Return the corresponding version 8 event flag or INVALID_EVENT_FLAG if not found")
			print("\treturn indexMap.find(v7) != indexMap.end() ? indexMap[v7] : INVALID_EVENT_FLAG;")
			print("}")

if __name__ == "__main__":
	import sys
	if len(sys.argv) < 3 or len(sys.argv) > 4:
		print("Usage: python parse_event_flags.py <branch1> <branch2> [list_diff]")
	else:
		# Determine if listing differences is required based on the third argument
		list_diff = len(sys.argv) == 4 and sys.argv[3] == 'list_diff'
		main(sys.argv[1], sys.argv[2], list_diff=list_diff)
