import os


for item in os.listdir('functional_test'):
    if item.endswith('.sy'):
        ret = os.system('cmake-build-debug/parser ' + os.path.abspath('./functional_test/' + item))
        if ret != 0:
            print('Error occurred in testcase ' + item)

