import pandas as pd

import warnings
warnings.simplefilter("ignore", FutureWarning)

print("Read preprocessed dataframe with pickle..")
df = pd.read_pickle("../output/processed_df.pkl")

# print("Exclude login node processes..")
# df = df[df["JOBIDRAW"] != -1]

print("Add unique process identifier..")
df["URowID"] = list(zip(df["JOBIDRAW"], df["STEPID"], df["PID"], df["HASH"], df["HOST"], df["TIME"]))

################################## Exclude Incomplete ##################################

print("Exclude incomplete df process rows..")
df = df[df['INFO'].notna()]
if 'OBJECTS' in df.columns:
    df = df[df['OBJECTS'].notna()]

print("Separate system and exclude incomplete..")
df_system = df[(df["system_vs_user"] == "system") & (~df["EXECUTABLE_basename"].str.startswith("python", na=False))]

print("Separate user and exclude incomplete..")
df_user = df[df["system_vs_user"] == "user"]
for col in ['MODULES', 'OBJECTS', 'COMPILERS', 'MAPS', 'FILE_H', 'STRINGS_H', 'SYMBOLS_H']:
    if col in df_user.columns:
        df_user = df_user[df_user[col].notna()]

print("Separate python and exclude incomplete..")
df_python = df[df["EXECUTABLE_basename"].str.startswith("python", na=False)]
if 'PYTHON_IMPORTS' in df_python.columns:
    df_python = df_python[df_python['PYTHON_IMPORTS'].notna()]
if 'PYTHON_FILE_H' in df_python.columns:
    df_python = df_python[df_python['PYTHON_FILE_H'].notna()]

################################## Check Truncation ##################################

truncation_cols = [c for c in df.columns if c.endswith('_TRUNCATED')]
if truncation_cols:
    print("# Truncated collector messages per TYPE..")
    truncation_counts = df[truncation_cols].notna().sum().rename("Processes")
    truncation_counts.index = truncation_counts.index.str.removesuffix('_TRUNCATED')
    truncation_counts.index.name = "TYPE"
    print(truncation_counts[truncation_counts > 0].to_string())
    print()

################################## First Row Example ##################################

print("")
print(f"# processed_df.pkl: shape {df.shape}")
print(f"# columns: {list(df.columns)}")
print()
print("# Example row (first row of df)")
key_width = max(len(c) for c in df.columns)
first = df.iloc[0]
for col in df.columns:
    print(f"{col:<{key_width}}  {first[col]}")
print()

################################## Print Process Overview ##########################

print("")
print("# Overview: users, jobs, processes")
jobs_counts = df.groupby("UID")["JOBIDRAW"].nunique().rename("Jobs")
system_counts = df_system.groupby("UID")["URowID"].nunique().rename("System P")
user_counts = df_user.groupby("UID")["URowID"].nunique().rename("User P")
python_counts = df_python.groupby("UID")["URowID"].nunique().rename("Python P")
results = pd.concat([jobs_counts, system_counts, user_counts, python_counts], axis=1).fillna(0).astype(int)
results = results.sort_values(["Jobs", "System P", "User P", "Python P"], ascending=False)
total_row = results.sum()
results = results.head(10)
results.loc["Total"] = total_row
print(results)
print()

################################## Print Job Overview ##############################

def fmt_list(items, sep=' | ', max_n=10):
    items = list(items)
    if len(items) <= max_n:
        return sep.join(items)
    return sep.join(items[:max_n]) + f' (+{len(items) - max_n} more)'

n_jobs_total = df['JOBIDRAW'].nunique()
print("")
print(f"# Overview: per-job detail (top 3 of {n_jobs_total} by process count)")
jobs_sorted = df.groupby('JOBIDRAW')['URowID'].nunique().sort_values(ascending=False).head(3)
trunc_cols = [c for c in df.columns if c.endswith('_TRUNCATED')]
for jobid in jobs_sorted.index:
    df_job = df[df['JOBIDRAW'] == jobid]
    df_job_user = df_user[df_user['JOBIDRAW'] == jobid]
    df_job_python = df_python[df_python['JOBIDRAW'] == jobid]

    uids = sorted(df_job['UID'].dropna().astype(str).unique())
    uid_str = ','.join(uids) if uids else '?'
    n_steps = df_job['STEPID'].nunique()
    n_hosts = df_job['HOST'].nunique()
    n_procs = df_job['URowID'].nunique()
    print(f"Job JOBIDRAW={jobid} (uid={uid_str}, {n_steps} steps, {n_hosts} hosts, {n_procs} procs)")

    top_bins = df_job_user.groupby('EXECUTABLE_basename')['URowID'].nunique().sort_values(ascending=False)
    if len(top_bins):
        bin_items = [f'{name}({cnt})' for name, cnt in top_bins.items()]
        print(f"  Top user bins:  {fmt_list(bin_items, '; ')}")

    if 'MODULES' in df_job_user.columns:
        modules = sorted(set(m.strip() for s in df_job_user['MODULES'].dropna() for m in s.split(';') if m.strip()))
        if modules:
            print(f"  Modules:        {fmt_list(modules)}")

    if 'COMPILERS' in df_job_user.columns:
        compilers = sorted(set(c.strip() for s in df_job_user['COMPILERS'].dropna() for c in s.split(';') if c.strip()))
        if compilers:
            print(f"  Compilers:      {fmt_list(compilers)}")

    if 'OBJECTS' in df_job_user.columns:
        objects = sorted(set(o.strip() for s in df_job_user['OBJECTS'].dropna() for o in s.split(';') if o.strip()))
        if objects:
            print(f"  Objects:        {fmt_list(objects)}")

    if not df_job_python.empty:
        py_scripts = sorted(df_job_python['PYTHON_SCRIPT_basename'].dropna().unique())
        if py_scripts:
            print(f"  Py scripts:     {fmt_list(py_scripts, ', ')}")
        if 'PYTHON_IMPORTS' in df_job_python.columns:
            imports = sorted(set(x.strip() for s in df_job_python['PYTHON_IMPORTS'].dropna() for x in s.split(';') if x.strip()))
            if imports:
                print(f"  Py imports:     {fmt_list(imports, '; ')}")

    trunc_summary = [f"{c.removesuffix('_TRUNCATED')}({df_job[c].notna().sum()})" for c in trunc_cols if df_job[c].notna().sum() > 0]
    if trunc_summary:
        print(f"  Truncations:    {fmt_list(trunc_summary, '; ')}")
    print()

################################## Check System ##################################

print("# Statistics for EXECUTABLE_basename in df_system..")
agg_dict = {'UID': pd.Series.nunique, 'JOBIDRAW': pd.Series.nunique, 'URowID': pd.Series.nunique}
if 'OBJECTS_H' in df_system.columns:
    agg_dict['OBJECTS_H'] = pd.Series.nunique
print(df_system.groupby('EXECUTABLE_basename').agg(agg_dict).reset_index().sort_values(list(agg_dict.keys()), ascending=False).reset_index(drop=True).head(10).to_string(index=False))
print()

################################## Check User ##################################

print("# Statistics for EXECUTABLE_basename in df_user..")
agg_dict = {'UID': pd.Series.nunique, 'JOBIDRAW': pd.Series.nunique, 'URowID': pd.Series.nunique}
if 'FILE_H' in df_user.columns:
    agg_dict['FILE_H'] = pd.Series.nunique
stats_user_exec = df_user.groupby('EXECUTABLE_basename').agg(agg_dict).reset_index()
stats_user_exec = stats_user_exec.sort_values(list(agg_dict.keys()), ascending=False).head(10)
print(stats_user_exec.to_string(index=False))
print()

print("# Statistics for MODULES in df_user..")
if 'MODULES' not in df_user.columns:
    print("(column MODULES not collected)")
else:
    agg_dict = {'UID': pd.Series.nunique, 'JOBIDRAW': pd.Series.nunique, 'URowID': pd.Series.nunique}
    if 'FILE_H' in df_user.columns:
        agg_dict['FILE_H'] = pd.Series.nunique
    stats_modules = df_user.groupby('MODULES').agg(agg_dict).reset_index()
    stats_modules = stats_modules.sort_values(list(agg_dict.keys()), ascending=False).head(10)
    print(stats_modules.to_string(index=False))
print()

print("# Statistics for OBJECTS in df_user..")
if 'OBJECTS' not in df_user.columns:
    print("(column OBJECTS not collected)")
else:
    df_user_objects = df_user.copy()
    df_user_objects['OBJECT'] = df_user_objects['OBJECTS'].fillna("").apply(lambda s: list(set(elem.strip() for elem in s.split(';') if elem.strip())))
    df_user_objects = df_user_objects.explode('OBJECT').dropna(subset=['OBJECT'])
    agg_dict = {'UID': pd.Series.nunique, 'JOBIDRAW': pd.Series.nunique, 'URowID': pd.Series.nunique}
    if 'FILE_H' in df_user.columns:
        agg_dict['FILE_H'] = pd.Series.nunique
    stats_objects = df_user_objects.groupby('OBJECT').agg(agg_dict).reset_index()
    stats_objects = stats_objects.sort_values(list(agg_dict.keys()), ascending=False).head(10)
    print(stats_objects.to_string(index=False))
print()

print("# Statistics for MAPS in df_user..")
if 'MAPS' not in df_user.columns:
    print("(column MAPS not collected)")
else:
    df_user_maps = df_user.copy()
    df_user_maps['MAP'] = df_user_maps['MAPS'].fillna("").apply(lambda s: list(set(elem.strip() for elem in s.split(';') if elem.strip())))
    df_user_maps = df_user_maps.explode('MAP').dropna(subset=['MAP'])
    agg_dict = {'UID': pd.Series.nunique, 'JOBIDRAW': pd.Series.nunique, 'URowID': pd.Series.nunique}
    if 'FILE_H' in df_user.columns:
        agg_dict['FILE_H'] = pd.Series.nunique
    stats_maps = df_user_maps.groupby('MAP').agg(agg_dict).reset_index()
    stats_maps = stats_maps.sort_values(list(agg_dict.keys()), ascending=False).head(10)
    print(stats_maps.to_string(index=False))
print()

print("# Statistics for COMPILERS in df_user..")
if 'COMPILERS' not in df_user.columns:
    print("(column COMPILERS not collected)")
else:
    agg_dict = {'UID': pd.Series.nunique, 'JOBIDRAW': pd.Series.nunique, 'URowID': pd.Series.nunique}
    if 'FILE_H' in df_user.columns:
        agg_dict['FILE_H'] = pd.Series.nunique
    stats_compilers = df_user.groupby('COMPILERS').agg(agg_dict).reset_index()
    stats_compilers = stats_compilers.sort_values(list(agg_dict.keys()), ascending=False).head(10)
    print(stats_compilers.to_string(index=False))
print()

################################## Check Python ##################################

print("# Statistics for PYTHON_IMPORTS in df_python..")
if df_python.empty or 'PYTHON_IMPORTS' not in df_python.columns:
    print("(no python processes captured)")
else:
    df_python_exploded = df_python.copy()
    df_python_exploded['PYTHON_IMPORT'] = df_python_exploded['PYTHON_IMPORTS'].fillna("").apply(lambda s: list(set(elem.strip() for elem in s.split(';') if elem.strip())))
    df_python_exploded = df_python_exploded.explode('PYTHON_IMPORT')
    agg_dict = {
        'UID': pd.Series.nunique,
        'JOBIDRAW': pd.Series.nunique,
        'URowID': pd.Series.nunique,
    }
    rename_map = {"UID": "Unique UIDs", "JOBIDRAW": "Jobs", "URowID": "Processes"}
    sort_cols = ["Unique UIDs", "Jobs", "Processes"]
    if 'PYTHON_FILE_H' in df_python_exploded.columns:
        agg_dict['PYTHON_FILE_H'] = pd.Series.nunique
        rename_map['PYTHON_FILE_H'] = "Unique PYTHON_FILE_H"
        sort_cols.append("Unique PYTHON_FILE_H")
    stats_python_imports = df_python_exploded.groupby('PYTHON_IMPORT').agg(agg_dict).reset_index().rename(columns=rename_map).sort_values(sort_cols, ascending=False).head(10)
    print(stats_python_imports.to_string(index=False))
print()

####################################################################################