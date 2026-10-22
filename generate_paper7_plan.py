import json
import csv
from pathlib import Path

PROJ_DIR = Path(r"C:\antigravity_goat\2026.22.10_Paper7_MAC_analog_Geant4")
STATE_FILE = Path(r"C:\antigravity_goat\control_center\projects_state.json")

SAMPLES = ["S0", "S1", "S2", "S3", "S4"]
ENERGIES = [
    122, 245, 296, 344, 411, 444, 679, 689, 779, 826,
    867, 964, 1005, 1086, 1112, 1173, 1299, 1408, 1458, 2506
]

all_jobs = []
for s in SAMPLES:
    for e in ENERGIES:
        job = {
            "job_name": f"{s}_1.5cm_{e}keV",
            "sample": s,
            "energy_kev": e,
            "thickness_cm": 1.5,
            "macro_file": f"run_{s}_D15_E{e}.mac",
            "csv_filename": f"{s}_1.5cm_{e}keV.csv",
            "root_filename": f"hist_{s}_D15_E{e}.root"
        }
        all_jobs.append(job)

# 1. Write manifest_paper7.csv
manifest_path = PROJ_DIR / "manifest_paper7.csv"
with open(manifest_path, "w", newline="", encoding="utf-8") as f:
    writer = csv.DictWriter(f, fieldnames=list(all_jobs[0].keys()))
    writer.writeheader()
    writer.writerows(all_jobs)
print(f"Created manifest_paper7.csv with {len(all_jobs)} jobs.")

# Also write checklist_paper7.csv to C:\antigravity_goat
checklist_path = Path(r"C:\antigravity_goat\checklist_paper7.csv")
with open(checklist_path, "w", newline="", encoding="utf-8") as f:
    writer = csv.DictWriter(f, fieldnames=list(all_jobs[0].keys()))
    writer.writeheader()
    writer.writerows(all_jobs)
print(f"Created checklist_paper7.csv with {len(all_jobs)} jobs.")

# 2. Round-Robin Plan across ACTIVE accounts
with open(STATE_FILE, "r", encoding="utf-8") as f:
    state_data = json.load(f)
accounts_dict = state_data.get("accounts", {})
active_accounts = [acc for acc, info in accounts_dict.items() if info.get("status") == "ACTIVE"]
print(f"Active accounts ({len(active_accounts)}): {active_accounts}")

num_runners = 10
plan = {acc: {str(r): [] for r in range(1, num_runners + 1)} for acc in active_accounts}

# Assign jobs round-robin
runner_slots = []
for acc in active_accounts:
    for r in range(1, num_runners + 1):
        runner_slots.append((acc, str(r)))

for idx, job in enumerate(all_jobs):
    acc, r_id = runner_slots[idx % len(runner_slots)]
    plan[acc][r_id].append(job)

plan_file = PROJ_DIR / "paper7_plan.json"
with open(plan_file, "w", encoding="utf-8") as f:
    json.dump(plan, f, indent=2, ensure_ascii=False)

print(f"Created plan file: {plan_file}")
