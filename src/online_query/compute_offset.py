import re
import sys

def main(argv):
    if (len(argv) != 2):
        print "[Usage] <input-parsed-file> <output-offset-file>"
        return
    input_file = argv[0]
    output_file = argv[1]

    with open(output_file, 'w') as output:
        with open(input_file, 'r') as input:
            for line in input:
                output.write(re.sub('[\[\]]', '', line))
                output.write('Offsets: ')
                offset = 0
                left = 0
                right = 0
                bias = 0
                for char in line:
                    if char == '[':
                        left = offset + 1
                    if char == ']':
                        right = offset
                        bias += 1
                        output.write('[' + str(left - bias * 2 + 1) + ', ' + str(right - bias * 2 + 1) + ']')
                        output.write(' (' + line[left:right] + '); ')
                    offset += 1
                output.write('\n')

if __name__ == "__main__":
    main(sys.argv[1 : ])
