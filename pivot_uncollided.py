"""
pivot_uncollided.py

Chuyen process_results.csv (dinh dang dai: moi dong la 1 run
Material,Thickness_mm,Energy_keV,...,N_uncollided,...)
thanh bang pivot: hang = (Thickness, Energy), cot = Material (s0, s1, s2, ...),
gia tri = N_uncollided.

Cach dung:
    python pivot_uncollided.py process_results.csv
(neu khong truyen tham so, mac dinh doc "process_results.csv" trong thu muc hien tai)

Ket qua:
    uncollided_pivot.csv
    uncollided_pivot.xlsx
"""

import sys
import pandas as pd

def main():
    infile = sys.argv[1] if len(sys.argv) > 1 else "process_results.csv"

    df = pd.read_csv(infile)

    required_cols = {"Material", "Thickness_mm", "Energy_keV", "N_uncollided"}
    missing = required_cols - set(df.columns)
    if missing:
        raise SystemExit(
            f"Thieu cot trong '{infile}': {missing}\n"
            f"Cac cot hien co: {list(df.columns)}"
        )

    df = df.rename(columns={"Thickness_mm": "Thickness", "Energy_keV": "Energy"})

    # Neu co nhieu run trung (Thickness, Energy, Material) -> lay gia tri dau tien.
    # Doi "first" thanh "mean" neu ban muon trung binh nhieu lan chay lai.
    pivot = df.pivot_table(
        index=["Thickness", "Energy"],
        columns="Material",
        values="N_uncollided",
        aggfunc="first",
    ).reset_index()

    pivot.columns.name = None  # bo ten "Material" hien tren dong header cua pandas

    # Sap xep cac cot vat lieu theo thu tu tu nhien: s0, s1, s2, s3, ...
    material_cols = [c for c in pivot.columns if c not in ("Thickness", "Energy")]
    material_cols = sorted(material_cols, key=lambda s: (len(s), s))
    pivot = pivot[["Thickness", "Energy"] + material_cols]

    pivot = pivot.sort_values(["Thickness", "Energy"]).reset_index(drop=True)

    pivot.to_csv("uncollided_pivot.csv", index=False)
    pivot.to_excel("uncollided_pivot.xlsx", index=False)

    print("Da tao: uncollided_pivot.csv va uncollided_pivot.xlsx\n")
    print(pivot.to_string(index=False))


if __name__ == "__main__":
    main()
