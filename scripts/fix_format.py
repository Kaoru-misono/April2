import os
import re

def fix_format(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    original_content = content

    # 1. Remove trailing whitespace from each line
    lines = content.splitlines()
    lines = [line.rstrip() for line in lines]
    content = '\n'.join(lines)

    # Ensure single newline at the end
    if content and not content.endswith('\n'):
        content += '\n'

    # 2. Convert tabs to 4 spaces
    content = content.replace('\t', '    ')

    # 3. East Const Enforcment (Simple Heuristics)
    # const Type& -> Type const&
    # const Type* -> Type const*
    # const Type  -> Type const (safe for simple types)
    
    # Regex for "const Type&" -> "Type const&"
    # Capture: const (whitespace) (Type) (& or *)
    # Avoid double const or cases where const is already correct
    
    # Pattern: \bconst\s+([a-zA-Z0-9_:]+(?:<[^>]+>)?)\s*(&|\*)
    # Replacement: \1 const\2
    content = re.sub(r'\bconst\s+([a-zA-Z0-9_:]+(?:<[^>]+>)?)\s*(&|\*)', r'\1 const\2', content)

    # Pattern: \bconst\s+([a-zA-Z0-9_:]+(?:<[^>]+>)?)\s+(?![&*])
    # Replacement: \1 const 
    # Be careful not to match "const int" in "const int&" again if regex engine overlaps, but Python's re usually consumes.
    # We will try a safe approach for standard types.
    
    # 4. Allman Style Braces
    # Look for ") {" or "){ " and replace with ")\n{" 
    # Exception: Lambdas "[]() {" -> "[]() {" (often kept inline or SameLine) 
    # But style guide says:
    # auto lambda = []() -> void {
    #     ...
    # };
    # So we should be careful. 
    # For functions: auto func() -> void { ... } (inline)
    # or
    # auto func() -> void
    # {
    # }
    #
    # Let's enforce the newline for function definitions not on a single line.
    
    # Replaces ") {" with ")\n{" generally.
    # content = re.sub(r'\)\s*\{', ')\n{', content) 
    # Wait, the guide says: "in header file, if function body less than one line: auto func() -> void { ... }"
    # So we cannot blindly replace ") {" with ")\n{".
    
    # However, we CAN enforce no trailing whitespace which we did. 
    
    # Let's focus on the "remove all whiteSpace" request from the user, 
    # which likely meant "trailing whitespace" and "formatting consistency".
    
    # 5. Spaces around template <>
    # template <typename T>
    content = re.sub(r'template<', 'template <', content)

    if content != original_content:
        print(f"Formatting: {file_path}")
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(content)

def main():
    root_dir = 'engine'
    print("Starting format fix...")
    
    for subdir, dirs, files in os.walk(root_dir):
        # Skip 'library' directories
        if 'library' in subdir.split(os.sep):
            continue
            
        # Modify dirs in-place to prevent walking into 'library'
        # This is for efficiency
        dirs[:] = [d for d in dirs if d != 'library']

        for file in files:
            if file.endswith('.cpp') or file.endswith('.hpp'):
                file_path = os.path.join(subdir, file)
                try:
                    fix_format(file_path)
                except Exception as e:
                    print(f"Error formatting {file_path}: {e}")

    print("Format fix complete.")

if __name__ == '__main__':
    main()
