import os
import re
import sys

def extract_strings_from_cpp(cpp_dirs):
    """Extract all string literals from .cpp files under given cpp_dirs."""
    strings = set()
    for cpp_dir in cpp_dirs:
        for root, _, files in os.walk(cpp_dir):
            for file in files:
                if file.endswith(".cpp"):
                    with open(os.path.join(root, file), 'r', encoding='utf-8', errors='replace') as f:
                        content = f.read()
                        # Find all string literals "...", accounting for escaped quotes
                        found = re.findall(r'"([^"\\]*(?:\\.[^"\\]*)*)"', content)
                        # Extract actual strings (found is a list of tuples)
                        for match in found:
                            strings.add(match)
    return strings

def line_matches_exception(line):
    """Check if the line matches any of the exception patterns."""
    exception_patterns = [
        r"sNewBox\d+",  # Matches sNewBox# where # is 1-20
        r"sNewBox\d+Name",  # Matches sNewBox#Name where # is 1-20
        r"sNewBox\d+Theme",  # Matches sNewBox#Theme where # is 1-20
        r"sBackupNewBox\d+",  # Matches sBackupNewBox# where # is 1-20
        r"sBackupNewBox\d+Name",  # Matches sBackupNewBox#Name where # is 1-20
        r"sBackupNewBox\d+Theme",  # Matches sBackupNewBox#Theme where # is 1-20
        r"wObject\d+Struct"  # Matches wObject#Struct where # is 1-12
    ]
    for pattern in exception_patterns:
        if re.search(pattern, line):
            return True
    return False

def filter_sym_file(input_sym, valid_strings, output_sym_filtered):
    """Filter lines in input_sym so that only labels present in valid_strings or exceptions remain."""
    with open(input_sym, 'r', encoding='utf-8') as f_in:
        lines = f_in.readlines()

    filtered_lines = []
    for line in lines:
        parts = line.strip().split(maxsplit=1)
        # Expect something like "00:0000 Label"
        if len(parts) == 2:
            address_part, label_part = parts
            if label_part in valid_strings or line_matches_exception(label_part):
                filtered_lines.append(line)

    # Write to output
    with open(output_sym_filtered, 'w', encoding='utf-8') as f_out:
        f_out.writelines(filtered_lines)

def main():
    """
    Usage:
        python filter_sym.py <input.sym> <output.sym.filtered>
    """
    if len(sys.argv) != 3:
        print("Usage: python filter_sym.py <input.sym> <output.sym.filtered>")
        sys.exit(1)

    input_sym = sys.argv[1]
    output_sym_filtered = sys.argv[2]

    # Directories containing the .cpp files whose string literals we want to allow
    cpp_dirs = ["src/patching", "src/core"]

    # Extract valid strings from all .cpp files in specified directories
    valid_strings = extract_strings_from_cpp(cpp_dirs)

    # Filter the symbol file
    filter_sym_file(input_sym, valid_strings, output_sym_filtered)

if __name__ == "__main__":
    main()
