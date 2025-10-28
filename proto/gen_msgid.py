#!/usr/bin/env python3

import re
import sys

def parse_proto_file(file_path):
    """Parse the proto file and extract oneof message definitions"""
    with open(file_path, 'r') as f:
        content = f.read()
    
    # Find the MsgBody oneof definition
    # This regex looks for oneof MsgBody block
    oneof_pattern = r'oneof\s+MsgBody\s*{([^}]+)}'
    oneof_match = re.search(oneof_pattern, content, re.DOTALL)
    
    if not oneof_match:
        print("Error: Could not find MsgBody oneof definition")
        return None
    
    oneof_content = oneof_match.group(1)
    
    # Extract message type and id from each line
    # Pattern to match: [type] [name] = [id];
    msg_pattern = r'(\w+)\s+(\w+)\s*=\s*(\d+)'
    messages = re.findall(msg_pattern, oneof_content)
    
    return messages

def generate_msgid_proto(messages, output_path):
    """Generate MSGID.proto file with enum definitions"""
    with open(output_path, 'w') as f:
        f.write('syntax = "proto3";\n\n')
        f.write('package LocalPhotoSync;\n\n')
        f.write('enum MSGID\n{\n')
        
        f.write(f"    ID_None = 0;\n")
        for msg_type, msg_name, msg_id in messages:
            # Convert CSReqDeviceInfo to ID_CSReqDeviceInfo
            enum_name = f"ID_{msg_name}"
            f.write(f"    {enum_name} = {msg_id};\n")
        
        f.write('}\n')

def main():
    if len(sys.argv) != 3:
        print("Usage: python3 parse_proto.py <input.proto> <output.proto>")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    
    messages = parse_proto_file(input_file)
    
    if messages:
        generate_msgid_proto(messages, output_file)
        print(f"Successfully generated {output_file}")
    else:
        print("Failed to parse proto file")
        sys.exit(1)

if __name__ == "__main__":
    main()