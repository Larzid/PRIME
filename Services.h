#ifndef SERVICES_H
#define SERVICES_H

struct shServiceDescData {
    char mDesc[40];
    int mNameKnown;
};
enum MedicalProcedures {
    kMedHealing = 0,
    kMedRestoration,
    kMedRectalExam,
    kMedDiagnostics,
    kMedRadPurification,
    kMedCaesareanSection,
    kMedCanisterAmputation,
    kMedTeefExtraction,
    kMedTailAmputation,
    kMedImplantExtraction,
    kMedMaxService
};
extern struct shServiceDescData MedicalProcedureData[kMedMaxService];

enum MelnormeServices {
    kMelnRevealBugginess,
    kMelnRevealCharges,
    kMelnRevealEnhancement,
    kMelnRevealAppearance,
    kMelnRandomIdentify,
    kMelnMaxService
};
/* Space reserved for six descriptions to have one more name to shuffle. */
extern struct shServiceDescData MelnormeServiceData[6 /* kMelnMaxService */ ];

#endif
