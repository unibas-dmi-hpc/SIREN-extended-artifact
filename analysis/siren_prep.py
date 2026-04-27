import re, os, sys
import pandas as pd
import numpy as np

print("# Read and preprocess captured SIREN output")
capture_file = '../output/captures.txt'
if not os.path.exists(capture_file):
    sys.exit(f"Capture file {capture_file} not found.")

print("Read capture file ../output/captures.txt..")
column_names = ['JOBIDRAW', 'STEPID', 'PID', 'HASH', 'HOST', 'TIME', 'LAYER', 'TYPE', 'CONTENT']
with open(capture_file) as f:
    rows = [line.rstrip('\n').split('|', 8) for line in f if line.strip()]
df = pd.DataFrame(rows, columns=column_names)
for col in column_names:
    df[col] = df[col].astype(str).str.removeprefix(f'{col}=')
df = df.sort_values(by=['JOBIDRAW', 'STEPID', 'PID', 'TIME'])

print("Separate hashes..")
mask = df['TYPE'].str.endswith('_1')
if mask.any():
    extracted_content = df.loc[mask, 'CONTENT'].str.split(';', n=1, expand=True)[0]
    df.loc[mask, 'CONTENT'] = df.loc[mask, 'CONTENT'].str.replace(r'^[^;]*;\s*', '', regex=True)
    new_entities = df.loc[mask].copy()
    new_entities['TYPE'] = new_entities['TYPE'].str.replace(r'_1$', '_H', regex=True)
    new_entities['CONTENT'] = extracted_content
    df = pd.concat([df, new_entities], ignore_index=True)

print("Process message chunks..")
mask = df['TYPE'].str.contains(r'_\d+$', regex=True)
df.loc[mask, 'BASE_TYPE'] = df.loc[mask, 'TYPE'].str.replace(r'_\d+$', '', regex=True)
df.loc[mask, 'CHUNK_ORDER'] = df.loc[mask, 'TYPE'].str.extract(r'_(\d+)$').astype(int)
group_cols = ['JOBIDRAW', 'STEPID', 'PID', 'HASH', 'HOST', 'TIME', 'LAYER', 'BASE_TYPE']
grouped = df.loc[mask].sort_values('CHUNK_ORDER').groupby(group_cols)
concatenated = grouped['CONTENT'].apply(''.join).reset_index()
concatenated = concatenated.rename(columns={'BASE_TYPE': 'TYPE'})
df = df.drop(df.loc[mask].index)
df = pd.concat([df, concatenated], ignore_index=True)
df = df.drop(columns=['BASE_TYPE', 'CHUNK_ORDER'], errors='ignore')

print("Consolidate messages, put all fields for one process in one row, keep earliest TIME..")
group_cols = ['JOBIDRAW', 'STEPID', 'PID', 'HASH', 'HOST', 'LAYER']
df_sorted = df.sort_values(by='TIME')
dup_groups = df_sorted.groupby(group_cols + ['TYPE']).size().gt(1).sum()
if dup_groups > 0:
    print(f"Warning: {dup_groups} duplicate (keys+TYPE) groups collapsed via aggfunc='last'")
df_min_time = df_sorted.groupby(group_cols, as_index=False)['TIME'].first()
df_pivot = df_sorted.pivot_table(index=group_cols, columns='TYPE', values='CONTENT', aggfunc='last').reset_index()
df = pd.merge(df_min_time, df_pivot, on=group_cols, how='left')

print("Merge python script pseudo-process-rows into parent-process-rows..")
keys = ['JOBIDRAW', 'STEPID', 'PID', 'HASH', 'HOST', 'TIME']
df_self = df[df['LAYER'] == 'SELF'].copy()
df_script = df[df['LAYER'] == 'SCRIPT'].copy()
script_non_key_cols = [col for col in df_script.columns if col not in keys]
df_script = df_script.rename(columns={col: f'PYTHON_{col}' for col in script_non_key_cols})
df = pd.merge(df_self, df_script, on=keys, how='left')
df = df.sort_values(by=['JOBIDRAW', 'STEPID', 'PID', 'TIME'], ascending=True)

print("Drop rows where INFO is NaN (possibly lost UDP packet)..")
df = df.dropna(subset=['INFO'])

print("Extract UID and EXE into own column..")
df['UID'] = df['INFO'].str.extract(r'UID:([^;]+)').fillna('')
df['EXECUTABLE'] = df['INFO'].str.extract(r'EXE:([^;]+)').fillna('')
df['EXECUTABLE_basename'] = df['EXECUTABLE'].apply(os.path.basename)

print("Extract PYTHON into its own columns..")
df['PYTHON_SCRIPT'] = df['PYTHON_INFO'].str.extract(r'EXE:([^;]+)').fillna('')
df['PYTHON_SCRIPT_basename'] = df['PYTHON_SCRIPT'].apply(os.path.basename)

print("Create PYTHON_IMPORTS column containing only imported python libraries..")
pattern = re.compile(r"(?:/site-packages/|/lib-dynload/)([^/\.]+)")
if 'MAPS' in df.columns:
    df['PYTHON_IMPORTS'] = df['MAPS'].apply(lambda s: ';'.join(sorted(set(match.group(1).replace('_', '') for token in s.split(';') for match in [pattern.search(token)] if match))) if isinstance(s, str) else '')

print("Drop unused, nan, and/or empty columns..")
df = df.drop(columns=["LAYER", "PYTHON_LAYER"])
df = df.dropna(axis=1, how='all').replace("", np.nan).dropna(axis=1, how='all')

print("Add system_vs_user column..")
system_prefixes = ("/etc/", "/dev/", "/usr/", "/bin/", "/boot/", "/lib/", "/opt/", "/sbin/", "/sys/", "/proc/", "/var/")
df["system_vs_user"] = np.where(df["EXECUTABLE"].str.startswith(system_prefixes), "system", "user")

print("Save the dataframe with pickle..")
df.to_pickle('../output/processed_df.pkl')

print("# Preprocessed and saved DataFrame")
print("")