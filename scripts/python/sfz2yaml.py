import yaml
import re

def parse_kv_line(line):
    parts = line.split('=', 1)
    if len(parts) != 2:
        return None, None
    key, value = parts[0].strip(), parts[1].strip()
    # แปลงค่าเป็นตัวเลขถ้าเป็นไปได้
    try:
        if '.' in value:
            value = float(value)
        else:
            value = int(value)
    except ValueError:
        pass
    key = key.lower().replace('-', '_')
    return key, value

def sfz_to_structured_yaml(sfz_path):
    result = {
        'global': {},
        'groups': []
    }
    current_section = None
    current_group = None
    current_region = None

    with open(sfz_path, 'r', encoding='utf-8') as f:
        for raw_line in f:
            line = raw_line.strip()
            if not line or line.startswith('//') or line.startswith('#'):
                continue
            if re.match(r'<global>', line, re.I):
                current_section = 'global'
                continue
            elif re.match(r'<group>', line, re.I):
                current_section = 'group'
                current_group = {
                    'name': None,
                    'regions': []
                }
                result['groups'].append(current_group)
                continue
            elif re.match(r'<region>', line, re.I):
                current_section = 'region'
                current_region = {}
                if current_group is None:
                    current_group = {'name': None, 'regions': []}
                    result['groups'].append(current_group)
                current_group['regions'].append(current_region)
                continue

            key, value = parse_kv_line(line)
            if key is None:
                continue

            # ถ้าอยู่ใน group แล้วเจอ key 'trigger' หรืออื่นๆ อาจจะเก็บเป็นชื่อกลุ่มด้วย
            if current_section == 'group':
                # สมมติ key 'trigger' หรือ 'group_name' ใช้แทนชื่อ group
                if key in ('trigger', 'group_name', 'name'):
                    current_group['name'] = str(value)
                else:
                    current_group[key] = value
            elif current_section == 'region':
                current_region[key] = value
            elif current_section == 'global':
                result['global'][key] = value

    # ถ้าเจอ group ไหนไม่มีชื่อ ให้ตั้งชื่อ default เป็น "GroupN"
    for idx, grp in enumerate(result['groups']):
        if not grp['name']:
            grp['name'] = f"Group{idx+1}"

    return result

def write_yaml(data, output_path):
    with open(output_path, 'w', encoding='utf-8') as f:
        yaml.dump(data, f, sort_keys=False, allow_unicode=True, indent=2)

if __name__ == '__main__':
    import sys
    if len(sys.argv) < 3:
        print("Usage: sfz2cleanyaml.py input.sfz output.yaml")
    else:
        data = sfz_to_structured_yaml(sys.argv[1])
        write_yaml(data, sys.argv[2])
