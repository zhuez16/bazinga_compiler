import os
import subprocess
start_from = 0
skip_list = []
os.chdir('cmake-build-debug')
l = os.listdir('../functional_test')
os.getcwdb()
l.sort()
for item in l:
    if item.endswith('.sy'):
        idx = item[:3]
        if idx[2] == '_':
            idx = idx[:2]
        if int(idx) >= start_from and int(idx) not in skip_list:
            executable_path = os.path.abspath('../functional_test/' + item[:-3])
            input_path = os.path.abspath('../functional_test/' + item[:-3] + '.in')
            output_path = os.path.abspath('../functional_test/' + item[:-3] + '.out')
            if os.path.exists(executable_path):
                os.remove(executable_path)
            ret = os.system('./compiler ' + os.path.abspath('../functional_test/' + item) + ' -const-propagation -code-sink')
            if ret != 0:
                print('Error occurred in building testcase ' + item)
                break
            input_option = None
            if os.path.exists(input_path):
                with open(input_path, 'rb') as f:
                    input_option = f.read()
            result = subprocess.run(executable_path, input=input_option, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=2)
            with open(output_path, 'rb') as f:
                correct = f.read()
                res = result.stdout
                res += str(result.returncode).encode('utf8')
                res = res.replace(b'\n', b'')
                correct = correct.replace(b'\n', b'')
                if res != correct:
                    print(f'Testcase {item} failed.[{res} != {correct}]')
                    break
                else:
                    print(f'Testcase {item} passed.')



