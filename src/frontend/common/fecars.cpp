#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "frontend/common/fecars.h"
#include "frontend/common/fecheats.h"
#include "frontend/common/femenu.h"
#include "frontend/common/fetourn.h"
#include "frontend/common/fetracks.h"
#include "game/common/nfs3.h"
#include "game/common/paths.h"
#include "lib/psx/blkmov.h"

class cars
{
public:
    static char disableBonuses;
};

enum tCarListType
{
    clt_Stock = 1,
    clt_GarageCar = 2,
    clt_Cop = 4,
    clt_Pursuit = 8,
    clt_Traffic = 16,
    clt_PinkSlips = 32,
    clt_Tournament = 64,
    clt_Dealer = 128
};

struct tTrackInformation
{
    char fTrackID;
    uint8 fSimNumber;
    uint8 fTrackDifficulty;
    uint8 fAvailable;
    uint8 fIsEgg;
    uint8 fLengthKM;
    uint8 fLengthMiles;
    uint8 fNumMoments;
    char fShapeName[8];
    char fSplineName[8];
    char fCountry;
    char fDispatch;
    char fReverseCall;
    char fLanguage;
    char fTrafficCars[6];
    sint16 fTX;
    sint16 fTY;
    sint16 fSX;
    sint16 fSY;
    uint8 fSpeedoCountry;
    uint8 fPad;
    sint32 fRotate;
};

char gCarSelected[2][50];
char cars::disableBonuses;

void tCarManager::Initialize(void)
{
    FUNCTION_MARKER(0x800161B0, "FRONT.BIN");
    fNumCars = 0;
    fCars = 0;
    for (sint16 j = 0; j < 2; ++j)
    {
        for (sint16 i = 0; i < 32; ++i)
        {
            fCarGarage[j][i].fCarID = (char)0xFF;
            fPinkSlipsCars[j][i].fCarID = (char)0xFF;
        }
    }
    for (sint16 i = 0; i < 48; ++i)
    {
        fAvailableCars[i] = 0;
        fViewableCars[i] = 0;
    }
}

tCarInfo *tCarManager::GetCarFromID(sint16 carID)
{
    FUNCTION_MARKER(0x8001624C, "FRONT.BIN");
    for (uint32 i = 0; i < fNumCars; ++i)
    {
        if ((sint8)fCars[i].fCarID == carID)
            return &fCars[i];
    }
    return 0;
}

tCarInfo *tCarManager::GetCarFromSimID(sint16 carID)
{
    FUNCTION_MARKER(0x8001629C, "FRONT.BIN");
    for (uint32 i = 0; i < fNumCars; ++i)
    {
        if (fCars[i].fSimNumber == carID)
            return &fCars[i];
    }
    return 0;
}

sint32 tCarManager::CheapestCarStockPrice(void)
{
    FUNCTION_MARKER(0x800162EC, "FRONT.BIN");
    sint32 returnprice = 10000000;
    for (uint32 i = 0; i < fNumCars; ++i)
    {
        sint32 carPrice = fCars[i].fPrices[0];
        if (carPrice > 0 && carPrice < returnprice)
            returnprice = carPrice;
    }
    return returnprice;
}

sint32 tCarManager::CalcUsedPrice(sint16 garageNumber)
{
    FUNCTION_MARKER(0x80016344, "FRONT.BIN");
    if ((uint32)(sint32)garageNumber < fNumCars)
        return 0;
    uint32 index = (uint32)(sint32)garageNumber - fNumCars;
    sint32 carID = (sint8)fCarGarage[index / 32][index % 32].fCarID;
    if (carID == -1)
        return 0;
    tCarInfo *carInfo = GetCarFromID((sint16)carID);
    if (carInfo == 0)
        return 0;
    index = (uint32)(sint32)garageNumber - fNumCars;
    uint8 upgrades = fCarGarage[index / 32][index % 32].fUpgrades;
    sint32 result = carInfo->fPrices[0];
    if (upgrades & 1)
        result = (sint32)((uint32)result + (uint32)carInfo->fPrices[1]);
    if (upgrades & 2)
        result = (sint32)((uint32)result + (uint32)carInfo->fPrices[2]);
    if (upgrades & 4)
        result = (sint32)((uint32)result + (uint32)carInfo->fPrices[3]);
    if (carInfo->fExoticCar)
        return (sint32)((uint32)result * 3) / 4;
    return result >> 1;
}

sint32 tCarManager::PurchaseCar(sint16 carModel, sint16 color, sint16 playerNum)
{
    FUNCTION_MARKER(0x80016450, "FRONT.BIN");
    tCarInfo *carInfo = carManager.GetCarFromID(carModel);
    for (sint16 i = 0; i < 32; ++i)
    {
        if ((sint8)fCarGarage[playerNum][i].fCarID < 0)
        {
            fCarGarage[playerNum][i].fCarID = (char)carModel;
            fCarGarage[playerNum][i].fCarColor = (uint8)color;
            fCarGarage[playerNum][i].fUpgrades = 0;
            frontEnd.garageCar[playerNum] = (char)(fNumCars + i);
            return carInfo->fPrices[0];
        }
    }
    return 0;
}

sint32 tCarManager::SellCar(sint16 garageNumber, sint16 playerNum)
{
    FUNCTION_MARKER(0x8001652C, "FRONT.BIN");
    uint32 index = (uint32)(sint32)garageNumber - fNumCars;
    if ((uint32)playerNum >= 2 || index >= 32)
        return 0;

    sint32 result = CalcUsedPrice(garageNumber);
    tOwnedCarInfo *garage = fCarGarage[playerNum];
    garage[index].fCarID = (char)0xFF;
    sint16 i = (sint16)(index + 1);
    while (i < 32 && (sint8)garage[i].fCarID >= 0)
    {
        tOwnedCarInfo &previous = garage[i - 1];
        previous.fCarID = garage[i].fCarID;
        previous.fUpgrades = garage[i].fUpgrades;
        previous.fCarColor = garage[i].fCarColor;
        ++i;
    }
    garage[i - 1].fCarID = (char)0xFF;
    garage[31].fCarID = (char)0xFF;

    uint8 selected = (uint8)frontEnd.garageCar[playerNum];
    if (selected >= fNumCars)
    {
        uint32 selectedIndex = selected - fNumCars;
        if (selectedIndex >= 32)
            frontEnd.garageCar[playerNum] = 0;
        else if ((sint8)garage[selectedIndex].fCarID < 0)
            frontEnd.garageCar[playerNum] = selectedIndex != 0 ? (char)(selected - 1) : 0;
    }
    return result;
}

sint32 tCarManager::PurchaseUpgrade(sint16 garageNumber, sint16 upgradeFlags, sint16 playerNum)
{
    FUNCTION_MARKER(0x800166AC, "FRONT.BIN");
    uint32 index = (uint32)(sint32)garageNumber - fNumCars;
    tCarInfo *carInfo = GetCarFromID((sint8)fCarGarage[playerNum][index].fCarID);
    sint32 result = 0;
    for (sint16 i = 0; i < 3; ++i)
    {
        sint16 mask = (sint16)(1 << i);
        if (upgradeFlags & mask)
        {
            index = (uint32)(sint32)garageNumber - fNumCars;
            uint8 &upgrades = fCarGarage[playerNum][index].fUpgrades;
            if (!(upgrades & mask))
            {
                upgrades |= (uint8)mask;
                result = (sint32)((uint32)result + (uint32)carInfo->fPrices[i + 1]);
            }
        }
    }
    return result;
}

void tCarManager::RemoveFromPinkSlipsList(sint16 garageNumber, sint16 playerNum)
{
    FUNCTION_MARKER(0x800167B8, "FRONT.BIN");
    uint32 index = uint32((sint32)garageNumber) - fNumCars;
    fPinkSlipsCars[playerNum][index].fCarID = (char)0xFF;
    sint16 i = (sint16)((uint16)garageNumber - (uint16)fNumCars + 1);
    while (i < 32)
    {
        tOwnedCarInfo &car = fPinkSlipsCars[playerNum][i];
        if ((sint8)car.fCarID < 0)
            break;
        tOwnedCarInfo &previous = fPinkSlipsCars[playerNum][i - 1];
        previous.fCarID = car.fCarID;
        previous.fUpgrades = car.fUpgrades;
        previous.fCarColor = car.fCarColor;
        i = (sint16)((uint16)i + 1);
    }
    fPinkSlipsCars[playerNum][i - 1].fCarID = (char)0xFF;
    fPinkSlipsCars[playerNum][31].fCarID = (char)0xFF;
    uint8 selected = frontEnd.garageCar[playerNum];
    uint32 selectedOffset = offsetof(tCarManager, fPinkSlipsCars) + playerNum * 128u +
        (uint32(selected) - fNumCars) * sizeof(tOwnedCarInfo);

    if (reinterpret_cast<const sint8 *>(this)[selectedOffset] < 0)
        frontEnd.pinkSlipsCar[playerNum] = (uint8)frontEnd.pinkSlipsCar[playerNum] > fNumCars ?
            (char)(selected - 1) : 0;
}

void tCarManager::AddToPinkSlipsList(sint16 carModel, sint16 color, sint16 playerNum)
{
    FUNCTION_MARKER(0x80016900, "FRONT.BIN");
    for (sint16 i = 0; i < 32; ++i)
    {
        if ((sint8)fPinkSlipsCars[playerNum][i].fCarID < 0)
        {
            fPinkSlipsCars[playerNum][i].fCarID = (char)carModel;
            fPinkSlipsCars[playerNum][i].fCarColor = (uint8)color;
            fPinkSlipsCars[playerNum][i].fUpgrades = 0;
            frontEnd.pinkSlipsCar[playerNum] = (char)(fNumCars + i);
            return;
        }
    }
}

void tCarManager::AddUpgradesToPinkSlipsList(sint16 garageNumber, sint16 upgradeFlags, sint16 playerNum)
{
    FUNCTION_MARKER(0x8001697C, "FRONT.BIN");
    uint32 index = (uint32)(sint32)garageNumber - fNumCars;
    GetCarFromID((sint8)fPinkSlipsCars[playerNum][index].fCarID);
    for (sint16 i = 0; i < 3; ++i)
    {
        sint16 mask = (sint16)(1 << i);
        if (upgradeFlags & mask)
        {
            index = (uint32)(sint32)garageNumber - fNumCars;
            uint8 &upgrades = fPinkSlipsCars[playerNum][index].fUpgrades;
            if (!(upgrades & mask))
                upgrades |= (uint8)mask;
        }
    }
}

void tCarManager::LoadCars(tSaveCarInfo &load, sint16 playerNum)
{
    FUNCTION_MARKER(0x80016A54, "FRONT.BIN");
    blockmove(load.fSaveInfo, fCarGarage[playerNum], 128);
    if (playerNum == 0)
    {
        for (uint32 i = 0; i < fNumCars; ++i)
        {
            fAvailableCars[i] = load.fSaveAvailable[i];
            fViewableCars[i] = load.fSaveViewable[i];
        }
    }
}

void tCarManager::SaveCars(tSaveCarInfo &save)
{
    FUNCTION_MARKER(0x80016AF0, "FRONT.BIN");
    blockmove(fCarGarage[0], save.fSaveInfo, 128);
    for (uint32 i = 0; i < fNumCars; ++i)
    {
        save.fSaveAvailable[i] = fAvailableCars[i];
        save.fSaveViewable[i] = fViewableCars[i];
    }
}

void tCarManager::LoadPinkSlipsCars(tSaveCarInfo &load, sint16 playerNum)
{
    FUNCTION_MARKER(0x80016B68, "FRONT.BIN");
    blockmove(load.fSaveInfo, fPinkSlipsCars[playerNum], 128);
    for (sint32 i = 0; (uint32)i < fNumCars; ++i)
    {
        fPinkSlipsAvailableCars[playerNum][i] = load.fSaveAvailable[i];
        fPinkSlipsViewableCars[playerNum][i] = load.fSaveViewable[i];
    }
}

void tCarManager::SavePinkSlipsCars(tSaveCarInfo &save, sint16 playerNum, sint16 withoutCarInGarageNumber)
{
    FUNCTION_MARKER(0x80016C0C, "FRONT.BIN");
    tCarInfo carInfo;
    if (withoutCarInGarageNumber != -1)
    {
        GetPinkSlipsCar(withoutCarInGarageNumber, carInfo, playerNum);
        RemoveFromPinkSlipsList(withoutCarInGarageNumber, playerNum);
    }
    blockmove(fPinkSlipsCars[playerNum], save.fSaveInfo, 128);
    for (sint32 i = 0; (uint32)i < fNumCars; ++i)
    {
        save.fSaveAvailable[i] = fPinkSlipsAvailableCars[playerNum][i];
        save.fSaveViewable[i] = fPinkSlipsViewableCars[playerNum][i];
    }
    if (withoutCarInGarageNumber != -1)
    {
        carManager.AddToPinkSlipsList((sint8)carInfo.fCarID, carInfo.fColor, playerNum);
        carManager.AddUpgradesToPinkSlipsList(frontEnd.pinkSlipsCar[playerNum], carInfo.fUpgrades, playerNum);
    }
}

void tCarManager::SetClassAvailable(tCarClassType carClass, bool avail)
{
    FUNCTION_MARKER(0x80016D68, "FRONT.BIN");
    for (uint32 i = 0; i < fNumCars; ++i)
    {
        sint32 carID = (sint8)fCars[i].fCarID;
        if (carID >= 0 && fCars[i].fCarClass == carClass)
        {
            if (carID < 48)
                fAvailableCars[carID] = avail;
            else if (carID < 96)
                fViewableCars[carID - 48] = avail;
            else
                fPinkSlipsAvailableCars[0][carID - 96] = avail;
            if (avail)
            {
                carID = (sint8)fCars[i].fCarID;
                if (carID < -48)
                    reinterpret_cast<uint8 *>(&fPinkSlipsCars)[carID + 304] = 1;
                else if (carID < 0)
                    fAvailableCars[carID + 48] = 1;
                else if (carID < 48)
                    fViewableCars[carID] = 1;
                else
                    fPinkSlipsAvailableCars[(carID - 48) / 48][(carID - 48) % 48] = 1;
            }
        }
    }
}

void tCarManager::SetCarAvailable(tCarModels carModel, bool avail)
{
    FUNCTION_MARKER(0x80016DEC, "FRONT.BIN");
    sint32 carID = carModel;
    if (carID < 0)
        reinterpret_cast<uint8 *>(&fPinkSlipsCars)[carID + 256] = avail;
    else if (carID < 48)
        fAvailableCars[carID] = avail;
    else if (carID < 96)
        fViewableCars[carID - 48] = avail;
    else
        fPinkSlipsAvailableCars[0][carID - 96] = avail;
    if (avail)
    {
        if (carID < -48)
            reinterpret_cast<uint8 *>(&fPinkSlipsCars)[carID + 304] = 1;
        else if (carID < 0)
            fAvailableCars[carID + 48] = 1;
        else if (carID < 48)
            fViewableCars[carID] = 1;
        else
            fPinkSlipsAvailableCars[(carID - 48) / 48][(carID - 48) % 48] = 1;
    }
}

void tCarManager::SetClassViewable(tCarClassType carClass, bool view)
{
    FUNCTION_MARKER(0x80016E08, "FRONT.BIN");
    for (uint32 i = 0; i < fNumCars; ++i)
    {
        sint32 carID = (sint8)fCars[i].fCarID;
        if (carID < -48)
            reinterpret_cast<uint8 *>(&fPinkSlipsCars)[carID + 304] = 0;
        else if (carID < 0)
            fAvailableCars[carID + 48] = 0;
        else if (fCars[i].fCarClass == carClass)
        {
            if (carID < 48)
                fViewableCars[carID] = view;
            else
                fPinkSlipsAvailableCars[(carID - 48) / 48][(carID - 48) % 48] = view;
        }
    }
}

void tCarManager::SetCarViewable(tCarModels carModel, bool view)
{
    FUNCTION_MARKER(0x80016E74, "FRONT.BIN");
    sint32 carID = carModel;
    if (carID < -48)
        reinterpret_cast<uint8 *>(&fPinkSlipsCars)[carID + 304] = view;
    else if (carID < 0)
        fAvailableCars[carID + 48] = view;
    else if (carID < 48)
        fViewableCars[carID] = view;
    else
        fPinkSlipsAvailableCars[(carID - 48) / 48][(carID - 48) % 48] = view;
    if (!view)
    {
        if (carID < 0)
            reinterpret_cast<uint8 *>(&fPinkSlipsCars)[carID + 256] = 1;
        else if (carID < 48)
            fAvailableCars[carID] = 1;
        else if (carID < 96)
            fViewableCars[carID - 48] = 1;
        else
            fPinkSlipsAvailableCars[0][carID - 96] = 1;
    }
}

void tCarManager::GetStockCar(sint16 carNumber, tCarInfo &carInfo)
{
    FUNCTION_MARKER(0x80016E90, "FRONT.BIN");
    if ((uint32)(sint32)carNumber < fNumCars)
    {
        blockmove(&fCars[carNumber], &carInfo, 204);
        sint32 carID = (sint8)carInfo.fCarID;
        uint8 available;
        if (carID < 0)
            available = reinterpret_cast<const uint8 *>(&fPinkSlipsCars)[carID + 256];
        else if (carID < 48)
            available = fAvailableCars[carID];
        else if (carID < 96)
            available = fViewableCars[carID - 48];
        else
            available = fPinkSlipsAvailableCars[0][carID - 96];
        carInfo.fAvailable = available;
        uint8 viewable;
        if (carID < -48)
            viewable = reinterpret_cast<const uint8 *>(&fPinkSlipsCars)[carID + 304];
        else if (carID < 0)
            viewable = fAvailableCars[carID + 48];
        else if (carID < 48)
            viewable = fViewableCars[carID];
        else
            viewable = fPinkSlipsAvailableCars[(carID - 48) / 48][(carID - 48) % 48];
        carInfo.fUpgrades = 0;
        carInfo.fCountry = 0;
        carInfo.fViewable = viewable;
        carInfo.fCarIndex = (uint8)carNumber;
    }
    else
    {
        GetGarageCar(carNumber, carInfo, 0);
        carInfo.fCarIndex = (uint8)carNumber;
    }
}

void tCarManager::GetGarageCar(sint16 garageNumber, tCarInfo &carInfo, sint16 playerNum)
{
    FUNCTION_MARKER(0x80016F4C, "FRONT.BIN");
    if (playerNum == 1 && GetNumOwnedCars(1) <= 0)
        playerNum = 0;
    uint32 index = (uint32)(sint32)garageNumber - fNumCars;
    tCarInfo *original = GetCarFromID((sint8)fCarGarage[playerNum][index].fCarID);
    blockmove(original, &carInfo, 204);
    carInfo.fAvailable = 1;
    carInfo.fViewable = 1;
    index = (uint32)(sint32)garageNumber - fNumCars;
    carInfo.fUpgrades = fCarGarage[playerNum][index].fUpgrades;
    index = (uint32)(sint32)garageNumber - fNumCars;
    uint8 color = fCarGarage[playerNum][index].fCarColor;
    carInfo.fCountry = 0;
    carInfo.fCarIndex = (uint8)garageNumber;
    carInfo.fColor = color;
}

void tCarManager::GetPinkSlipsCar(sint16 garageNumber, tCarInfo &carInfo, sint16 playerNum)
{
    FUNCTION_MARKER(0x80017058, "FRONT.BIN");
    uint32 index = (uint32)(sint32)garageNumber - fNumCars;
    tCarInfo *original = GetCarFromID((sint8)fPinkSlipsCars[playerNum][index].fCarID);
    blockmove(original, &carInfo, 204);
    carInfo.fAvailable = 1;
    carInfo.fViewable = 1;
    index = (uint32)(sint32)garageNumber - fNumCars;
    carInfo.fUpgrades = fPinkSlipsCars[playerNum][index].fUpgrades;
    index = (uint32)(sint32)garageNumber - fNumCars;
    uint8 color = fPinkSlipsCars[playerNum][index].fCarColor;
    carInfo.fCountry = 0;
    carInfo.fCarIndex = (uint8)garageNumber;
    carInfo.fColor = color;
}

void tCarManager::LoadDescription(void)
{
    FUNCTION_MARKER(0x80017134, "FRONT.BIN");
    char filename[80];
    sprintf(filename, "%s%s", Paths_Paths[37], "fecars.car");
    ReleaseDescription();
    char *input = static_cast<char *>(loadfileadr(filename, 16));
    memcpy(&fNumCars, input, sizeof(fNumCars));
    fCars = static_cast<tCarInfo *>(reservememadr("Car List", fNumCars * 204, 0));
    char *data = input + 4;
    blockmove(data, fCars, fNumCars * 204);
    for (sint16 i = 0; (uint32)i < fNumCars; ++i)
    {
        if (fCars[i].fAvailable)
        {
            sint32 carID = (sint8)fCars[i].fCarID;
            if (carID < 0)
                reinterpret_cast<uint8 *>(&fPinkSlipsCars)[carID + 256] = 1;
            else if (carID < 48)
                fAvailableCars[carID] = 1;
            else if (carID < 96)
                fViewableCars[carID - 48] = 1;
            else
                fPinkSlipsAvailableCars[0][carID - 96] = 1;
            carID = (sint8)fCars[i].fCarID;
            if (carID < -48)
                reinterpret_cast<uint8 *>(&fPinkSlipsCars)[carID + 304] = 1;
            else if (carID < 0)
                fAvailableCars[carID + 48] = 1;
            else if (carID < 48)
                fViewableCars[carID] = 1;
            else
                fPinkSlipsAvailableCars[(carID - 48) / 48][(carID - 48) % 48] = 1;
        }
        for (sint16 j = 0; j < fCars[i].fNumLightColors + fCars[i].fNumDarkColors; ++j)
        {
            if ((sint8)fCars[i].fColorOrder[j] == fCars[i].fDefaultColor)
            {
                fCars[i].fDefaultColor = (uint8)j;
                break;
            }
        }
    }
    purgememadr(input);
}

void tCarManager::ReleaseDescription(void)
{
    FUNCTION_MARKER(0x800172F4, "FRONT.BIN");
    if (fCars != 0)
        purgememadr(fCars);
    fCars = 0;
    fNumCars = 0;
}

sint16 tCarManager::GetNumOwnedCars(sint16 playerNum)
{
    FUNCTION_MARKER(0x80017334, "FRONT.BIN");
    sint16 num = 0;
    for (sint32 i = 0; i < 32; ++i)
    {
        if ((sint8)fCarGarage[playerNum][i].fCarID >= 0)
            ++num;
    }
    return num;
}

sint16 tCarManager::GetNumTourneyCars(sint16 playerNum)
{
    FUNCTION_MARKER(0x80017378, "FRONT.BIN");
    sint16 result = 0;
    tCarInfo carInfo;
    for (sint32 i = 0; i < 32; ++i)
    {
        sint32 carID = (sint8)fCarGarage[playerNum][i].fCarID;
        if (carID >= 0)
        {
            carInfo.fCarID = fCarGarage[playerNum][i].fCarID;
            carInfo.fUpgrades = fCarGarage[playerNum][i].fUpgrades;
            tCarInfo *original = GetCarFromID(carID);
            carInfo.fCarClass = original->fCarClass;
            if (tournamentManager.ValidCar(carInfo))
                ++result;
        }
    }
    return result;
}

sint16 tCarManager::GetNumPinkSlipsCars(sint16 playerNum)
{
    FUNCTION_MARKER(0x80017420, "FRONT.BIN");
    sint16 num = 0;
    for (sint32 i = 0; i < 32; ++i)
    {
        if ((sint8)fPinkSlipsCars[playerNum][i].fCarID >= 0)
            ++num;
    }
    return num;
}

sint16 tCarManager::GetClassList(tCarClassType carClass, sint16 numElements, tCarModels *models)
{
    FUNCTION_MARKER(0x80017464, "FRONT.BIN");
    sint16 numCars = 0;
    uint16 i = 0;
    if (fNumCars != 0)
    {
        do
        {
            if (fCars[i].fCarClass == carClass)
            {
                if (numCars < numElements)
                    models[numCars] = (tCarModels)(sint8)fCars[i].fCarID;
                ++numCars;
            }
            ++i;
        } while (i < fNumCars);
    }
    if ((uint16)numCars < numElements)
    {
        if (numCars == 0)
            abort();
        for (i = (uint16)numCars; i < numElements; ++i)
            models[i] = models[i % numCars];
    }
    return numCars;
}

void tCarManager::InitializeIngameCarList(void)
{
    FUNCTION_MARKER(0x80017578, "FRONT.BIN");
    for (sint16 i = 0; i < 2; ++i)
    {
        for (sint16 j = 0; j < 50; ++j)
            gCarSelected[i][j] = 0;
    }
}

bool tCarManager::IsCarAnAddedModel(tCarModels &model, char &color)
{
    FUNCTION_MARKER(0x800175F8, "FRONT.BIN");
    tCarInfo *carInfo = GetCarFromID((sint16)model);
    sint32 carColor = color == 16 ? (sint8)carInfo->fPursuitAvailable : (sint8)carInfo->fColorOrder[color];
    sint32 colorGroup = carColor / 8;
    return gCarSelected[colorGroup][model] != 0;
}

void tCarManager::AddCarToIngameList(tCarModels &model, char &color)
{
    FUNCTION_MARKER(0x80017678, "FRONT.BIN");
    tCarInfo *carInfo = GetCarFromID((sint16)model);
    sint16 carColor = color == 16 ? (sint8)carInfo->fPursuitAvailable : (sint8)carInfo->fColorOrder[color];
    gCarSelected[carColor / 8][model] |= (char)(1 << (carColor & 7));
}

bool tCarManager::FindSimilarCar(tCarModels &model, char &color, sint16, tCarModels *)
{
    FUNCTION_MARKER(0x80017718, "FRONT.BIN");
    tCarInfo *carInfo = GetCarFromID(model);
    sint16 colorScheme = (sint8)carInfo->fColorOrder[color] / 8;
    sint16 numColors = carInfo->fNumLightColors + carInfo->fNumDarkColors;
    for (sint16 j = 0; j < 2; ++j)
    {
        for (sint16 i = 0; i < numColors; ++i)
        {
            sint32 index = (i + color) % numColors;
            uint8 carColor = carInfo->fColorOrder[index];
            sint32 group = carColor >> 3;
            if (group == colorScheme)
            {
                uint8 selected = gCarSelected[group][model];
                sint32 mask = 1 << (carColor & 7);
                if (!(selected & mask))
                {
                    gCarSelected[group][model] = (char)(selected | mask);
                    color = (char)((i + color) % numColors);
                    return true;
                }
            }
        }
        colorScheme = 1 - colorScheme;
    }
    return true;
}

tListIteratorCar::tListIteratorCar(char *valPtr, tCarManager *carManager) : tListIterator(0, valPtr)
{
    FUNCTION_MARKER(0x800178CC, "FRONT.BIN");
    fCarManager = carManager;
    fCarListFilter = 1;
    fNameLength = cnl_Medium;
}

tListIteratorCar::~tListIteratorCar(void)
{
    FUNCTION_MARKER(0x80017928, "FRONT.BIN");
}

char tListIteratorCar::Value(tPlayer atIndex)
{
    FUNCTION_MARKER(0x80017950, "FRONT.BIN");
    sint16 i = atIndex == kPlayerBoth ? 0 : (sint16)atIndex;
    return fValue[i];
}

sint16 tListIteratorCar::TextValue(tPlayer atIndex)
{
    FUNCTION_MARKER(0x8001797C, "FRONT.BIN");
    sint16 nameBase[3] = { 289, 339, 389 };
    sint16 i = atIndex == kPlayerBoth ? 0 : (sint16)atIndex;
    uint8 carNumber = fValue[i];
    sint32 carID;
    if (carNumber < fCarManager->fNumCars)
        carID = (sint8)fCarManager->fCars[carNumber].fCarID;
    else
    {
        uint32 index = carNumber - fCarManager->fNumCars;
        if (fCarListFilter & 32)
            carID = (sint8)fCarManager->fPinkSlipsCars[i][index].fCarID;
        else
            carID = (sint8)fCarManager->fCarGarage[i][index].fCarID;
    }
    return nameBase[fNameLength] + carID;
}

void tListIteratorCar::AdjustPosition(tPlayer atIndex, sint16 direction)
{
    FUNCTION_MARKER(0x80017A9C, "FRONT.BIN");
    sint16 i = atIndex == kPlayerBoth ? 0 : (sint16)atIndex;
    char oldValue = fValue[i];
    char oldCountry = 0;
    tCarInfo *carInfo = 0;
    if ((uint8)fValue[i] < fCarManager->fNumCars)
    {
        carInfo = &fCarManager->fCars[(uint8)fValue[i]];
        oldCountry = frontEnd.carCountry[i][(sint8)carInfo->fCarID];
    }
    sint32 firstCar = 0;
    sint32 lastCar;
    if ((fCarListFilter & 0x89) != 0)
        lastCar = (sint32)fCarManager->fNumCars;
    else
    {
        firstCar = (sint32)fCarManager->fNumCars;
        tOwnedCarInfo *ownedCars;
        if ((fCarListFilter & 0x42) != 0)
        {
            ownedCars = fCarManager->fCarGarage[i];
            if (i == 1 && fCarManager->GetNumOwnedCars(1) <= 0)
                ownedCars = fCarManager->fCarGarage[0];
        }
        else
            ownedCars = fCarManager->fPinkSlipsCars[i];
        lastCar = 0;
        while ((sint8)ownedCars[lastCar].fCarID >= 0)
            ++lastCar;
        lastCar = (sint32)(uint32(lastCar) + uint32(firstCar));
    }
    if ((uint8)fValue[i] >= lastCar)
        fValue[i] = (char)(uint32(lastCar) - 1u);
    if ((uint8)fValue[i] < firstCar)
        fValue[i] = (char)firstCar;
    if (firstCar >= lastCar)
        return;
    for (;;)
    {
        uint8 number = fValue[i];
        if (number < fCarManager->fNumCars)
        {
            carInfo = &fCarManager->fCars[number];
            if (carInfo->fCarClass == 7)
            {
                frontEnd.carCountry[i][(sint8)carInfo->fCarID] =
                    (char)((uint8)frontEnd.carCountry[i][(sint8)carInfo->fCarID] + direction);
                if ((sint8)frontEnd.carCountry[i][(sint8)carInfo->fCarID] >= 5)
                {
                    frontEnd.carCountry[i][(sint8)carInfo->fCarID] = 0;
                    fValue[i] = (char)((uint8)fValue[i] + 1);
                    if ((uint8)fValue[i] < lastCar)
                    {
                        carInfo = &fCarManager->fCars[(uint8)fValue[i]];
                        frontEnd.carCountry[i][(sint8)carInfo->fCarID] = 0;
                    }
                }
                else if ((sint8)frontEnd.carCountry[i][(sint8)carInfo->fCarID] < 0)
                {
                    frontEnd.carCountry[i][(sint8)carInfo->fCarID] = 4;
                    fValue[i] = (char)((uint8)fValue[i] - 1);
                    if ((uint8)fValue[i] >= firstCar)
                    {
                        carInfo = &fCarManager->fCars[(uint8)fValue[i]];
                        frontEnd.carCountry[i][(sint8)carInfo->fCarID] = 4;
                    }
                }
            }
            else
            {
                frontEnd.carCountry[i][(sint8)carInfo->fCarID] = direction > 0 ? 0 : 4;
                fValue[i] = (char)((uint8)fValue[i] + direction);
                if (direction > 0 && (sint8)fValue[i] < lastCar)
                {
                    carInfo = &fCarManager->fCars[(uint8)fValue[i]];
                    frontEnd.carCountry[i][(sint8)carInfo->fCarID] = 0;
                }
                else if (direction < 0 && (sint8)fValue[i] >= firstCar)
                {
                    carInfo = &fCarManager->fCars[(uint8)fValue[i]];
                    frontEnd.carCountry[i][(sint8)carInfo->fCarID] = 4;
                }
            }
        }
        else
            fValue[i] = (char)(number + direction);
        if ((sint8)fValue[i] >= lastCar)
        {
            fValue[i] = (char)firstCar;
            if ((uint8)fValue[i] < fCarManager->fNumCars)
            {
                carInfo = &fCarManager->fCars[(uint8)fValue[i]];
                frontEnd.carCountry[i][(sint8)carInfo->fCarID] = 0;
            }
        }
        if ((sint8)fValue[i] < firstCar)
        {
            fValue[i] = (char)(uint32(lastCar) - 1u);
            if ((uint8)fValue[i] < fCarManager->fNumCars)
            {
                carInfo = &fCarManager->fCars[(uint8)fValue[i]];
                frontEnd.carCountry[i][(sint8)carInfo->fCarID] = 4;
            }
        }
        if (oldValue == fValue[i] && ((sint8)fValue[i] >= (sint32)fCarManager->fNumCars ||
            (sint8)frontEnd.carCountry[i][(sint8)carInfo->fCarID] == (uint8)oldCountry))
            return;
        if (ValidCar(atIndex, fValue[i]))
            return;
    }
}

void tListIteratorCar::Increment(tPlayer atIndex)
{
    FUNCTION_MARKER(0x80018094, "FRONT.BIN");
    AdjustPosition(atIndex, 1);
}

void tListIteratorCar::Decrement(tPlayer atIndex)
{
    FUNCTION_MARKER(0x800180B4, "FRONT.BIN");
    AdjustPosition(atIndex, -1);
}

bool tListIteratorCar::ValidCar(tPlayer atIndex, char carNumber)
{
    FUNCTION_MARKER(0x800180D4, "FRONT.BIN");
    sint16 i = atIndex == kPlayerBoth ? 0 : (sint16)atIndex;
    sint16 k = i;
    if (i == 1 && fCarManager->GetNumOwnedCars(1) <= 0)
        k = 0;
    uint8 number = carNumber;
    if (number >= fCarManager->fNumCars)
    {
        uint8 owned = (uint8)(number - (uint8)fCarManager->fNumCars);
        if ((fCarListFilter & 0x20) != 0)
            return (sint8)fCarManager->fPinkSlipsCars[i][owned].fCarID >= 0;
        if ((fCarListFilter & 0x42) == 0)
            return false;
        sint16 carID = (sint8)fCarManager->fCarGarage[k][owned].fCarID;
        bool result = carID >= 0;
        tCarInfo *carInfo = fCarManager->GetCarFromID(carID);
        if (frontEnd.raceType == 1 && carInfo->fPursuitAvailable == 0)
            return false;
        if (!result)
            return false;
        if ((fCarListFilter & 0x40) == 0)
            return result;
        tCarInfo garageCar;
        fCarManager->GetGarageCar((sint16)(owned + (uint16)fCarManager->fNumCars), garageCar, 0);
        return tournamentManager.ValidCar(garageCar);
    }
    tCarInfo *carInfo = &fCarManager->fCars[number];
    sint16 carID = (sint8)carInfo->fCarID;
    if (carID < 0 || fCarManager->fViewableCars[carID] == 0)
        return false;
    if (frontEnd.raceType == 1 && carInfo->fPursuitAvailable == 0)
        return false;
    carInfo = &fCarManager->fCars[number];
    bool result = false;
    if (carInfo->fCarClass == 7)
    {
        if ((fCarListFilter & 0xC) != 0)
        {
            char *country = &frontEnd.carCountry[i][carID];
            if (((uint32)carInfo->fCountries >> (uint32((sint8)*country) & 31)) & 1)
            {
                tTrackInformation trackInfo;
                trackManager.GetTrack((uint8)frontEnd.track[0], trackInfo);
                if (FECheat::IsCheatEnabled(cheat_AllCops) || (uint8)trackInfo.fCountry == (sint8)*country)
                    result = true;
            }
        }
    }
    else if (carInfo->fCarClass < 8)
    {
        result = (fCarListFilter & 0x81) != 0;
        if (carID == 0x1C && (frontEnd.carListType == 1 || frontEnd.gameMode == 1 || frontEnd.raceType != 0))
            result = false;
    }
    else if (carInfo->fCarClass == 8 && (fCarListFilter & 0x10) != 0)
        result = true;
    if (!result || (fCarListFilter & 1) == 0 || frontEnd.raceType != 2)
        return result;
    return tournamentManager.ValidCar(fCarManager->fCars[number]);
}

tListIteratorCarColor::tListIteratorCarColor(char *value, char *player, char *playerCar, sint32 indexSize, tCarManager *carManager) : tListIterator(0, value)
{
    FUNCTION_MARKER(0x800184A0, "FRONT.BIN");
    fPlayer = player;
    fPlayerCar = playerCar;
    fIndexSize = indexSize;
    fCarManager = carManager;
}

tListIteratorCarColor::~tListIteratorCarColor(void)
{
    FUNCTION_MARKER(0x80018520, "FRONT.BIN");
}

char tListIteratorCarColor::Value(tPlayer)
{
    FUNCTION_MARKER(0x80018548, "FRONT.BIN");
    uint8 player = *fPlayer;
    sint32 offset = (sint32)((uint32)player * (uint32)fIndexSize);
    tCarInfo *carInfo = &fCarManager->fCars[fPlayerCar[player]];
    offset = (sint32)((uint32)offset + (uint32)(sint8)carInfo->fCarID);
    return fValue[offset];
}

sint16 tListIteratorCarColor::TextValue(tPlayer)
{
    FUNCTION_MARKER(0x800185A8, "FRONT.BIN");
    return 0;
}

void tListIteratorCarColor::Increment(tPlayer)
{
    FUNCTION_MARKER(0x800185B0, "FRONT.BIN");
    uint8 player = *fPlayer;
    sint32 offset = (sint32)((uint32)player * (uint32)fIndexSize);
    tCarInfo *carInfo = &fCarManager->fCars[fPlayerCar[player]];
    char *value = fValue + (sint32)((uint32)offset + (uint32)(sint8)carInfo->fCarID);
    *value = (char)(*value + 1);
    value = fValue + (sint32)((uint32)offset + (uint32)(sint8)carInfo->fCarID);
    if (*value >= carInfo->fNumLightColors + carInfo->fNumDarkColors)
        *value = 0;
}

void tListIteratorCarColor::Decrement(tPlayer)
{
    FUNCTION_MARKER(0x80018648, "FRONT.BIN");
    uint8 player = *fPlayer;
    sint32 offset = (sint32)((uint32)player * (uint32)fIndexSize);
    tCarInfo *carInfo = &fCarManager->fCars[fPlayerCar[player]];
    char *value = fValue + (sint32)((uint32)offset + (uint32)(sint8)carInfo->fCarID);
    uint8 current = *value;
    if (current == 0)
        *value = (char)(carInfo->fNumLightColors + carInfo->fNumDarkColors - 1);
    else
        *value = (char)(current - 1);
}
