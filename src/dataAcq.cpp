#include "dataAcq.h"

void ReadBMPSensor(SFE_BMP180 sensor, double* temp, double* pres)
{
    char status;
    status=sensor.startTemperature();
    if (status !=0)
    {
        delay(status);
        status=sensor.getTemperature(*temp);
        status=sensor.startPressure(3);
        if (status!=0)
        {
            delay(status);
            status=sensor.getPressure(*pres,*temp);

        }
    }
}

