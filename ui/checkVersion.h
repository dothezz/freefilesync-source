#ifndef UPDATEVERSION_H_INCLUDED
#define UPDATEVERSION_H_INCLUDED


namespace FreeFileSync
{
void checkForUpdateNow();

void checkForUpdatePeriodically(long& lastUpdateCheck);
}

#endif // UPDATEVERSION_H_INCLUDED
