import os

testcase_path = './functional_test'

testcase_path = os.path.abspath(testcase_path)
os.chdir('cmake-build-debug')
files = os.listdir(testcase_path)
files.sort()
for item in files:
    if item.endswith('.sy'):
        command = f'./compiler -S -o {os.path.join(testcase_path, item[:-3] + ".s")} {os.path.join(testcase_path, item)}'
        # print('Executing command ' + command)
        ret = os.system(command)
        if ret != 0:
            print(f"Error occurred while generating {item[:-3]}.s")
        else:
            print(f"Assembly for {item} generated successfully")
