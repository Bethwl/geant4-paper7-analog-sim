// =========================================================================
// MyDetectorConstruction.cc  —  REAL-TRANSPORT uncollided-flux benchmark
//
// World + Sample + TallySurface (thin invisible volume right behind Sample).
//
// Particles are no longer killed on first interaction inside the Sample.
// They are transported normally through the full physics list (Compton,
// Rayleigh, photoelectric, pair production, secondaries, everything).
// "Uncollided" is now decided in MySteppingAction by comparing the
// primary's current kinetic energy to its vertex (birth) energy at the
// moment it enters the TallySurface volume.
//
// Hình học vẫn khớp với PHITS mẫu:
//   PHITS "10  pz 0.0"          -> mặt trước Sample tại z = 0
//   PHITS "19  pz <thickness>"  -> mặt sau Sample tại z = thickness
//   PHITS "20  cz 5.0"          -> bán kính Sample = 5 cm
// TallySurface được đặt ngay sau mặt "19" (cách 1 um) để không nhiễu hình học.
// =========================================================================

#include "MyDetectorConstruction.hh"

#include "G4NistManager.hh"          // Material database
#include "G4Box.hh"                  // World box
#include "G4Tubs.hh"                 // Sample / Tally cylinder
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include "G4Region.hh"
#include "G4RegionStore.hh"
#include "G4ProductionCuts.hh"
#include "G4SystemOfUnits.hh"
#include "G4RunManager.hh"

// =========================================================================
// Geometry Dimensions
// =========================================================================
namespace GEO {
    const G4double WorldSize = 1.0 * m;    // world đủ lớn bao quanh Sample
    const G4double Sample_Radius = 5.0 * cm;   // khớp PHITS "20  cz 5.0"
    const G4double TallyHalfThickness = 1.0 * um;  // rất mỏng, không ảnh hưởng transport
}

// =========================================================================
// Constructor / Destructor
// =========================================================================
MyDetectorConstruction::MyDetectorConstruction()
    : G4VUserDetectorConstruction(),
    fMessenger(nullptr),
    fSample_Thickness(5.0 * mm),   // giá trị mặc định, ghi đè bằng macro cho từng run
    fSampleMaterial("S1")          // vật liệu mặc định, ghi đè bằng macro cho từng run
{
    fMessenger = new G4GenericMessenger(this, "/DetectorConstruction/", "Detector Construction Control");
    fMessenger->DeclareProperty("Sample_Thickness", fSample_Thickness, "Set Sample_Thickness");
    fMessenger->DeclareProperty("Sample_Material", fSampleMaterial, "Set Sample material name (e.g. S1, S2, S3...)");
    fMessenger->DeclareMethod("update", &MyDetectorConstruction::UpdateGeometry, "Update geometry after parameter changes");
}

MyDetectorConstruction::~MyDetectorConstruction() {}

// =========================================================================
// Define Materials
// =========================================================================
void MyDetectorConstruction::DefineMaterials() {
    G4NistManager* NIST = G4NistManager::Instance();

    // World: chân không
    NIST->FindOrBuildMaterial("G4_Galactic");

    // ===============================================
    // Sample Materials — theo COMPOSITION / DENSITY goc (KHONG chuan hoa)
    // ===============================================
    //
    // ZAID -> nguyen to (theo chuan ACE/ENDF ZZZAAA.xxc, "000" = tu nhien):
    //   8000=O  11000=Na  12000=Mg  13000=Al  14000=Si
    //   16000=S 19000=K   20000=Ca  26000=Fe  30000=Zn  82000=Pb
    //
    // LUU Y: tong phan tram khoi luong CUA MOT SO vat lieu KHONG bang 1.0
    // (S1: 0.9642, S2: 0.8900, S3: 0.9020, S4: 0.8811) - giu nguyen theo
    // yeu cau, khong chuan hoa lai.

    // [0]. S0: "No sample" placeholder - vacuum
    if (!G4Material::GetMaterial("S0", false)) {
        new G4Material("S0", 1, 1.008 * g / mole, 1e-25 * g / cm3,
            kStateGas, 2.73 * kelvin, 3e-18 * pascal);
    }

    // [1]. S1: C0.0Te — density 2.26 g/cm3
    if (!G4Material::GetMaterial("S1", false)) {
        G4Material* S1 = new G4Material("S1", 2.26 * g / cm3, 11);
        S1->AddElement(NIST->FindOrBuildElement("Si"), 0.3838);
        S1->AddElement(NIST->FindOrBuildElement("Ca"), 0.4790);
        S1->AddElement(NIST->FindOrBuildElement("Al"), 0.0614);
        S1->AddElement(NIST->FindOrBuildElement("Fe"), 0.03001);
        S1->AddElement(NIST->FindOrBuildElement("C"), 0.0200);
        S1->AddElement(NIST->FindOrBuildElement("K"), 0.00772);
        S1->AddElement(NIST->FindOrBuildElement("Na"), 0.00802);
        S1->AddElement(NIST->FindOrBuildElement("Mg"), 0.00063);
        S1->AddElement(NIST->FindOrBuildElement("S"), 0.00423);
        S1->AddElement(NIST->FindOrBuildElement("Mn"), 0.00313);
        S1->AddElement(NIST->FindOrBuildElement("Ti"), 0.00202);
    }

    // [2]. S2: C0.5Te — density 2.28 g/cm3
    if (!G4Material::GetMaterial("S2", false)) {
        G4Material* S2 = new G4Material("S2", 2.28 * g / cm3, 10);
        S2->AddElement(NIST->FindOrBuildElement("Si"), 0.3816);
        S2->AddElement(NIST->FindOrBuildElement("Ca"), 0.4772);
        S2->AddElement(NIST->FindOrBuildElement("Al"), 0.05832);
        S2->AddElement(NIST->FindOrBuildElement("Fe"), 0.02546);
        S2->AddElement(NIST->FindOrBuildElement("C"), 0.02071);
        S2->AddElement(NIST->FindOrBuildElement("K"), 0.01178);
        S2->AddElement(NIST->FindOrBuildElement("Na"), 0.00855);
        S2->AddElement(NIST->FindOrBuildElement("Mg"), 0.00608);
        S2->AddElement(NIST->FindOrBuildElement("S"), 0.00266);
        S2->AddElement(NIST->FindOrBuildElement("Te"), 0.00760);
    }

    // [3]. S3: C1.0Te — density 2.46 g/cm3
    if (!G4Material::GetMaterial("S3", false)) {
        G4Material* S3 = new G4Material("S3", 2.46 * g / cm3, 10);
        S3->AddElement(NIST->FindOrBuildElement("Si"), 0.3815);
        S3->AddElement(NIST->FindOrBuildElement("Ca"), 0.4638);
        S3->AddElement(NIST->FindOrBuildElement("Al"), 0.06385);
        S3->AddElement(NIST->FindOrBuildElement("Fe"), 0.02466);
        S3->AddElement(NIST->FindOrBuildElement("C"), 0.02045);
        S3->AddElement(NIST->FindOrBuildElement("K"), 0.01185);
        S3->AddElement(NIST->FindOrBuildElement("Na"), 0.00860);
        S3->AddElement(NIST->FindOrBuildElement("Mg"), 0.00612);
        S3->AddElement(NIST->FindOrBuildElement("S"), 0.00210);
        S3->AddElement(NIST->FindOrBuildElement("Te"), 0.01706);
    }

    // [4]. S4: C1.5Te — density 2.49 g/cm3 (khop file mau: "C1.5Te Cylinder")
    if (!G4Material::GetMaterial("S4", false)) {
        G4Material* S4 = new G4Material("S4", 2.49 * g / cm3, 10);
        S4->AddElement(NIST->FindOrBuildElement("Si"), 0.3812);
        S4->AddElement(NIST->FindOrBuildElement("Ca"), 0.4586);
        S4->AddElement(NIST->FindOrBuildElement("Al"), 0.05689);
        S4->AddElement(NIST->FindOrBuildElement("Fe"), 0.02340);
        S4->AddElement(NIST->FindOrBuildElement("C"), 0.02085);
        S4->AddElement(NIST->FindOrBuildElement("K"), 0.01208);
        S4->AddElement(NIST->FindOrBuildElement("Na"), 0.00955);
        S4->AddElement(NIST->FindOrBuildElement("Mg"), 0.00643);
        S4->AddElement(NIST->FindOrBuildElement("S"), 0.00312);
        S4->AddElement(NIST->FindOrBuildElement("Te"), 0.02796);
    }

    // [5]. S7: Pure Lead (Pb) — density 11.34 g/cm3
    if (!G4Material::GetMaterial("S7", false)) {
        G4Material* S7 = new G4Material("S7", 11.34 * g / cm3, 1);
        S7->AddElement(NIST->FindOrBuildElement("Pb"), 1.0);
    }
}

// =========================================================================
// Construct Geometry
// =========================================================================
G4VPhysicalVolume* MyDetectorConstruction::Construct() {
    DefineMaterials();
    G4NistManager* NIST = G4NistManager::Instance();

    // ===============================================
    // 1. World Volume
    // ===============================================
    G4Box* solidWorld = new G4Box("World", GEO::WorldSize / 2, GEO::WorldSize / 2, GEO::WorldSize / 2);
    G4LogicalVolume* World_LV = new G4LogicalVolume(solidWorld, NIST->FindOrBuildMaterial("G4_Galactic"), "WorldLV");
    World_Phys = new G4PVPlacement(nullptr, {}, World_LV, "World", nullptr, false, 0, true);

    // ===============================================
    // 2. Sample
    // ===============================================
    // Mặt trước đặt đúng tại z = 0 (giống PHITS surface "10  pz 0.0"),
    // mặt sau tại z = fSample_Thickness (giống PHITS surface "19  pz thickness").
    G4Material* sampleMat = G4Material::GetMaterial(fSampleMaterial, false);
    if (!sampleMat) {
        G4cerr << "\n*** ERROR: Sample material \"" << fSampleMaterial
            << "\" not found (did you define it in DefineMaterials()?). "
            << "Falling back to S1.\n" << G4endl;
        sampleMat = G4Material::GetMaterial("S1");
    }

    G4Tubs* solidSample = new G4Tubs("Sample", 0, GEO::Sample_Radius, fSample_Thickness / 2, 0, 360 * deg);
    G4LogicalVolume* Sample_LV = new G4LogicalVolume(solidSample, sampleMat, "Sample_LV");
    G4double Sample_PosZ = fSample_Thickness / 2;  // mặt trước rơi đúng vào z = 0
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, Sample_PosZ), Sample_LV, "Sample", World_LV, false, 0, true);

    // ===============================================
    // 3. TallySurface — mặt ảo rất mỏng, đặt ngay sau Sample
    // ===============================================
    // NEW: Thay vì kill hạt ngay khi tương tác trong Sample, giờ ta để hạt
    // transport thật sự (đầy đủ Compton, Rayleigh, quang điện, pair, cùng
    // toàn bộ secondary). Volume này chỉ dùng để MySteppingAction phát hiện
    // "hạt vừa vào TallySurface" và kiểm tra năng lượng còn nguyên vẹn hay
    // không (so với vertex energy) để phân loại uncollided.
    G4Tubs* solidTally = new G4Tubs("TallySurface", 0, GEO::Sample_Radius,
        GEO::TallyHalfThickness, 0, 360 * deg);
    G4LogicalVolume* Tally_LV = new G4LogicalVolume(
        solidTally, NIST->FindOrBuildMaterial("G4_Galactic"), "TallyLV");
    fTallyLV = Tally_LV;

    G4double Tally_PosZ = fSample_Thickness + GEO::TallyHalfThickness;  // sát ngay mặt sau Sample
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, Tally_PosZ), Tally_LV,
        "TallySurface", World_LV, false, 0, true);

    // ===============================================
    // VISUALIZATION
    // ===============================================
    G4VisAttributes* World_VisAtt = new G4VisAttributes(G4Colour(1, 1, 1, 0.0));
    World_VisAtt->SetVisibility(false);
    World_LV->SetVisAttributes(World_VisAtt);

    G4VisAttributes* Sample_VisAtt = new G4VisAttributes(G4Colour(0, 1, 1));
    Sample_VisAtt->SetForceSolid(true);
    Sample_LV->SetVisAttributes(Sample_VisAtt);

    G4VisAttributes* Tally_VisAtt = new G4VisAttributes(G4Colour(1, 0, 0, 0.3));
    Tally_VisAtt->SetVisibility(false);   // để invisible; đổi true nếu muốn debug hình học
    Tally_LV->SetVisAttributes(Tally_VisAtt);

    // ===============================================
    // Production cuts thấp trong Sample — quan trọng để mô phỏng
    // đúng vận chuyển photon năng lượng thấp (khớp XCOM).
    // ===============================================
    G4Region* Sample_Region = G4RegionStore::GetInstance()->FindOrCreateRegion("SampleRegion");
    Sample_Region->AddRootLogicalVolume(Sample_LV);
    G4ProductionCuts* sampleCuts = new G4ProductionCuts();
    sampleCuts->SetProductionCut(1 * nm, G4ProductionCuts::GetIndex("gamma"));
    sampleCuts->SetProductionCut(1 * nm, G4ProductionCuts::GetIndex("e-"));
    sampleCuts->SetProductionCut(1 * nm, G4ProductionCuts::GetIndex("e+"));
    Sample_Region->SetProductionCuts(sampleCuts);

    return World_Phys;
}

void MyDetectorConstruction::UpdateGeometry() {
    auto runManager = G4RunManager::GetRunManager();
    runManager->ReinitializeGeometry();
    ConstructSDandField();
    G4cout << "\n================ Updated Geometry ================\n";
    G4cout << " Sample Thickness: " << fSample_Thickness / cm << " cm\n";
    G4cout << " Sample Material : " << fSampleMaterial << "\n";
}

G4String MyDetectorConstruction::GetSampleMaterialName() const {
    return fSampleMaterial;
}

G4double MyDetectorConstruction::GetSampleThickness() const {
    return fSample_Thickness;
}

// =========================================================================
// Construct Sensitive Detectors
// =========================================================================
// Vẫn không cần sensitive detector: TallySurface không phải SD, việc đếm
// "uncollided" hoàn toàn nằm trong MySteppingAction (check tên volume +
// so sánh năng lượng track hiện tại với vertex energy).
void MyDetectorConstruction::ConstructSDandField() {
    // (để trống có chủ đích)
}
