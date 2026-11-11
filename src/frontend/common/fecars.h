#ifndef FECARS_H
#define FECARS_H

#include "xport.h"
#include "frontend/common/tcarinfo.h"

enum tCarClassType
{
    cct_Roadster = 0,
    cct_PonyCar = 1,
    cct_SaloonCar = 2,
    cct_SportsCar = 3,
    cct_SuperCar = 4,
    cct_GTRCar = 5,
    cct_BonusCar = 6,
    cct_CopCar = 7,
    cct_TrafficCar = 8,
    cct_Helicopter = 9,
    cct_OpenClass = 10,
    cct_NumCarClasses = 11,
};

enum tCarModels
{
    cm_MercedesSLK = 0,
    cm_BMWZ3 = 1,
    cm_HoldenHSVT = 2,
    cm_FordFalcon = 3,
    cm_ChevyCamaro = 4,
    cm_TransAm = 5,
    cm_AstonMartinDB7 = 6,
    cm_JaguarXKR = 7,
    cm_BMWM5 = 8,
    cm_ChevyCorvette = 9,
    cm_Ferrari550 = 10,
    cm_Porsche911 = 11,
    cm_FerrariF50 = 12,
    cm_DiabloSV = 13,
    cm_MercedesCLK = 14,
    cm_McLarenF1 = 15,
    cm_XPerimentalPorsche = 16,
    cm_XPerimentalF50 = 17,
    cm_XPerimentalF1 = 18,
    cm_BonusCar1 = 19,
    cm_BonusCar2 = 20,
    cm_BonusCar3 = 21,
    cm_CapriceCop = 22,
    cm_HoldenHSVTCop = 23,
    cm_BMWM5Cop = 24,
    cm_CorvetteCop = 25,
    cm_PorscheCop = 26,
    cm_DiabloCop = 27,
    cm_Helicopter = 28,
    cm_AudiA4 = 29,
    cm_BMW530i = 30,
    cm_Mercedes500SL = 31,
    cm_VolvoWagon = 32,
    cm_SchoolBus = 33,
    cm_LandCruiser = 34,
    cm_Miata = 35,
    cm_ArmyTruck = 36,
    cm_JeepCommanche = 37,
    cm_OneTonTruck = 38,
    cm_OneTonTruckDirty = 39,
    cm_MercedesVan = 40,
    cm_JeepCherokee = 41,
    cm_Satellite = 42,
    cm_OldChevyPickup = 43,
    cm_FordF150 = 44,
    cm_Barracuda = 45,
    cm_TaxiCab = 46,
    cm_TourBus = 47,
    cm_LoggingTruck = 48,
    cm_TransitBus = 49,
    cm_NumCarModels = 50,
};

struct tOwnedCarInfo
{
    char fCarID;
    uint8 fUpgrades;
    uint8 fCarColor;
    uint8 fPad;
};

struct tSaveCarInfo
{
    tOwnedCarInfo fSaveInfo[32];
    uint8 fSaveAvailable[48];
    uint8 fSaveViewable[48];
};

class tCarManager
{
public:
    uint32 fNumCars;
    tCarInfo *fCars;
    tOwnedCarInfo fCarGarage[2][32];
    tOwnedCarInfo fPinkSlipsCars[2][32];
    uint8 fAvailableCars[48];
    uint8 fViewableCars[48];
    uint8 fPinkSlipsAvailableCars[2][48];
    uint8 fPinkSlipsViewableCars[2][48];
    sint16 fCarTextList[49];
    void Initialize(void);
    tCarInfo *GetCarFromID(sint16 carID);
    tCarInfo *GetCarFromSimID(sint16 carID);
    sint32 CheapestCarStockPrice(void);
    sint32 CalcUsedPrice(sint16 garageNumber);
    sint32 PurchaseCar(sint16 carModel, sint16 color, sint16 playerNum);
    sint32 SellCar(sint16 garageNumber, sint16 playerNum);
    sint32 PurchaseUpgrade(sint16 garageNumber, sint16 upgradeFlags, sint16 playerNum);
    void RemoveFromPinkSlipsList(sint16 garageNumber, sint16 playerNum);
    void AddToPinkSlipsList(sint16 carModel, sint16 color, sint16 playerNum);
    void AddUpgradesToPinkSlipsList(sint16 garageNumber, sint16 upgradeFlags, sint16 playerNum);
    void LoadCars(tSaveCarInfo &load, sint16 playerNum);
    void SaveCars(tSaveCarInfo &save);
    void LoadPinkSlipsCars(tSaveCarInfo &load, sint16 playerNum);
    void SavePinkSlipsCars(tSaveCarInfo &save, sint16 playerNum, sint16 withoutCarInGarageNumber);
    void SetClassAvailable(tCarClassType carClass, bool avail);
    void SetCarAvailable(tCarModels carModel, bool avail);
    void SetClassViewable(tCarClassType carClass, bool view);
    void SetCarViewable(tCarModels carModel, bool view);
    void GetStockCar(sint16 carNumber, tCarInfo &carInfo);
    void GetGarageCar(sint16 garageNumber, tCarInfo &carInfo, sint16 playerNum);
    void GetPinkSlipsCar(sint16 garageNumber, tCarInfo &carInfo, sint16 playerNum);
    void LoadDescription(void);
    void ReleaseDescription(void);
    sint16 GetNumOwnedCars(sint16 playerNum);
    sint16 GetNumTourneyCars(sint16 playerNum);
    sint16 GetNumPinkSlipsCars(sint16 playerNum);
    sint16 GetClassList(tCarClassType carClass, sint16 numElements, tCarModels *models);
    void InitializeIngameCarList(void);
    bool IsCarAnAddedModel(tCarModels &model, char &color);
    void AddCarToIngameList(tCarModels &model, char &color);
    bool FindSimilarCar(tCarModels &model, char &color, sint16, tCarModels *);
};

#endif
