import sys
import os

def elf_to_c_string(elf_filepath, output_filepath=None):
    """
    逐字节读取.elf文件并将其内容输出为C字符串格式。
    如果指定了输出文件路径，则将结果写入文件；否则，返回字符串。

    参数:
    elf_filepath (str): .elf文件的路径。
    output_filepath (str, optional): 结果输出文件的路径。如果为None，则函数返回字符串。

    返回:
    str: 如果output_filepath为None，则返回包含文件内容的C字符串。
         如果output_filepath不为None，则返回None（结果已写入文件）。
         如果文件不存在或无法读取，则返回None。
    """
    if not os.path.exists(elf_filepath):
        print(f"错误: 文件 '{elf_filepath}' 不存在。", file=sys.stderr)
        return None
    if not os.path.isfile(elf_filepath):
        print(f"错误: '{elf_filepath}' 不是一个文件。", file=sys.stderr)
        return None

    c_string_parts = []
    try:
        with open(elf_filepath, 'rb') as f:
            while True:
                byte = f.read(1) # Read byte by byte
                if not byte:
                    break # End of file
                # Convert byte to hexadecimal string and format as \xXX for C string
                c_string_parts.append(f"\\x{byte[0]:02x}")
        
        # Join all parts to form the final C string
        c_str = "".join(c_string_parts)

        if output_filepath:
            try:
                with open(output_filepath, 'w') as out_f:
                    out_f.write(c_str)
                print(f"C字符串已成功写入到文件: '{output_filepath}'")
                return None # Indicate that output was written to file
            except IOError as e:
                print(f"错误: 无法写入输出文件 '{output_filepath}': {e}", file=sys.stderr)
                return None
        else:
            return c_str
    except IOError as e:
        print(f"错误: 无法读取文件 '{elf_filepath}': {e}", file=sys.stderr)
        return None
    except Exception as e:
        print(f"发生意外错误: {e}", file=sys.stderr)
        return None

if __name__ == "__main__":
    # Check command line arguments
    if len(sys.argv) < 2:
        print("用法: python elf_to_c_string.py <elf_file_path> [output_file_path]")
        print("示例 (输出到控制台): python elf_to_c_string.py my_program.elf")
        print("示例 (输出到文件): python elf_to_c_string.py my_program.elf output.txt")
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = None
    if len(sys.argv) > 2:
        output_file = sys.argv[2]
    
    # Call the function for conversion
    c_str = elf_to_c_string(input_file, output_file)

    if c_str is not None: # Only print if output_file was not specified
        print(f"文件 '{input_file}' 的C字符串表示:")
        print(c_str)
