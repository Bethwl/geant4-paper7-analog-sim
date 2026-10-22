// =========================================================================
// MySteppingAction.cc  —  REAL-TRANSPORT / UNCOLLIDED-FLUX MODE
//
// KHÁC với bản cũ (kill-on-first-interaction):
//   - Hạt (primary) được transport THẬT SỰ trong Sample: Compton, Rayleigh,
//     quang điện, pair production, và toàn bộ secondary đều xảy ra tự nhiên
//     theo physics list, KHÔNG bị kill sớm.
//   - "Uncollided" được xác định bằng cách so sánh động năng hiện tại của
//     primary với động năng lúc sinh ra (vertex energy) tại thời điểm hạt
//     bước vào volume "TallySurface" (mặt ảo mỏng ngay sau Sample).
//     Nếu năng lượng KHÔNG đổi (trong sai số rất nhỏ) -> hạt chưa hề mất
//     năng lượng -> uncollided (kể cả Rayleigh scattering, vì Rayleigh
//     không đổi năng lượng, đúng theo định nghĩa T-Cross ctmin/ctmax=0
//     của PHITS mà ta đang so sánh).
//   - Vẫn giữ lại (tuỳ chọn) thống kê loại tương tác ĐẦU TIÊN của primary
//     trong Sample (Compton/Photoelectric/Pair/Rayleigh/Other) để tính tỷ
//     lệ R như bản cũ — nhưng KHÔNG kill hạt nữa sau khi ghi nhận.
// =========================================================================

#include "MySteppingAction.hh"
#include "MyRunAction.hh"
#include "MyEventAction.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4VProcess.hh"
#include "G4VPhysicalVolume.hh"
#include "G4Gamma.hh"
#include "G4SystemOfUnits.hh"

#include <cmath>

MySteppingAction::MySteppingAction(MyRunAction* runAction, MyEventAction* eventAction)
    : fRunAction(runAction), fEventAction(eventAction)
{
}

void MySteppingAction::UserSteppingAction(const G4Step* step)
{
    auto track = step->GetTrack();

    // Chỉ theo dõi hạt primary (trackID == 1). Không cần lọc theo loại hạt
    // gamma ở đây vì primary luôn là gamma theo PrimaryGeneratorAction,
    // nhưng kiểm tra tường minh để code rõ ràng và an toàn nếu sau này đổi
    // loại hạt nguồn.
    if (track->GetTrackID() != 1) return;
    if (track->GetDefinition() != G4Gamma::Gamma()) return;

    auto prePoint = step->GetPreStepPoint();
    auto preVolume = prePoint->GetPhysicalVolume();
    if (!preVolume) return;

    // ---------------------------------------------------------------
    // 1) (Tuỳ chọn) Ghi nhận loại tương tác ĐẦU TIÊN trong Sample, để
    //    vẫn có thống kê Compton/Photoelectric/Pair/Rayleigh như bản cũ.
    //    KHÔNG kill hạt — để nó tiếp tục transport thật.
    // ---------------------------------------------------------------
    if (preVolume->GetName() == "Sample") {
        auto process = step->GetPostStepPoint()->GetProcessDefinedStep();
        G4String procName = process ? process->GetProcessName() : "";

        if (procName != "Transportation" && procName != "CoupledTransportation" &&
            fEventAction && !fEventAction->IsFirstInteractionRecorded()) {
            fRunAction->AddProcessCount(procName);
            fEventAction->SetFirstInteractionRecorded();
        }
        // Không return, không kill — hạt tiếp tục chạy bình thường.
    }

    // ---------------------------------------------------------------
    // 2) Hạt vừa BƯỚC VÀO TallySurface (mặt ảo ngay sau Sample):
    //    so sánh động năng hiện tại với động năng lúc sinh ra (vertex).
    //    Nếu không đổi -> uncollided.
    // ---------------------------------------------------------------
    if (preVolume->GetName() == "TallySurface") {
        const G4double ekinNow    = track->GetKineticEnergy();
        const G4double ekinVertex = track->GetVertexKineticEnergy();

        // Sai số rất nhỏ để tránh vấn đề làm tròn số thực (double precision).
        const G4double tolerance = 1.0 * eV;

        if (std::fabs(ekinNow - ekinVertex) < tolerance) {
            fRunAction->AddUncollided();
        }

        // Đã đến Tally rồi thì không cần theo dõi thêm -> kill để tiết kiệm CPU.
        track->SetTrackStatus(fStopAndKill);
        return;
    }
}
