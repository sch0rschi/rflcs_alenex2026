import random
import os
import time

def generate_random_string(length, unique_numbers):
    # Generate a random string of given length from 0 to unique_numbers-1
    random.seed(time.time())
    return ' '.join(str(random.randint(0, unique_numbers - 1)) for _ in range(length))

def generate_file_content(unique_numbers, string_length, num_strings):
    content = f"2\t{unique_numbers}\n"
    for _ in range(num_strings):
        content += f"{string_length}\n"
        content += generate_random_string(string_length, unique_numbers) + '\n'
    return content

def generate_files(num_files, unique_numbers=24, string_length=32, num_strings=2, output_dir='generated_instances'):
    # Ensure the output directory exists
    os.makedirs(output_dir, exist_ok=True)

    for i in range(num_files):
        file_content = generate_file_content(unique_numbers, string_length, num_strings)
        file_name = f"{string_length}_{unique_numbers}.{i}"
        file_path = os.path.join(output_dir, file_name)
        with open(file_path, 'w') as file:
            file.write(file_content)

if __name__ == "__main__":
    # Parameters
    num_files = 1
    string_length = 16
    unique_numbers = 5
    num_strings = 2

    # Generate files
    generate_files(num_files, unique_numbers, string_length, num_strings)
