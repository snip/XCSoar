import sys
import time
import datetime

def main():
    if (len(sys.argv) < 3):
        return

    in_file = open(sys.argv[1], 'r')

    lines = []
    header = True
    for line in in_file:
        if header:
            if line.strip() is '':
                header = False
            
            continue

        if line.startswith('Version '):
            index = line.find(' - ')
            if index is -1:
                break

            index = index + 3
            date = line[index:].strip()
            if 'not yet released' not in date:
                date = date.replace('-', '/').split('/')
                date = datetime.date(int(date[0]), int(date[1]), int(date[2]))
                today = datetime.date.today()
                if (today - date).days > 90:
                    break

        lines.append(line)
    
    in_file.close()
    
    if len(lines) is 0:
        return

    out_file = open(sys.argv[2], 'w')
    for line in lines:
        out_file.write(line)
    out_file.close()        

if __name__ == '__main__':
    main()
