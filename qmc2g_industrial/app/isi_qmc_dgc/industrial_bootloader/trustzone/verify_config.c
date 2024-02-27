/***********************************************************************************************************************
 * Included files
 **********************************************************************************************************************/
#include "fsl_common.h"
#include "verify_config.h"
#include "qmc2_se.h"

// 12 MDA registers + 128 PDAP registers + 33 MR-MRSA/MREA/MRC
uint32_t sha512msg[173] = {0};

/***********************************************************************************************************************
 * BOARD_Init(uint32_t)RDC function
 **********************************************************************************************************************/
sss_status_t Verify_Rdc_Config(uint8_t *cmpSha512)
{
    volatile uint32_t i,j = 0;
    sss_status_t status = kStatus_SSS_Fail;
    uint8_t sha512[64];

	assert(cmpSha512 != NULL);

    memset(sha512msg, 0, sizeof(sha512msg));
    memset(sha512, 0, sizeof(sha512));

    for(i=0; i<12; i++)
    {
    	sha512msg[j++] = (uint32_t)RDC->MDA[i];
    }

    sha512msg[j++] = (uint32_t)RDC->MR[44].MRSA;
    sha512msg[j++] = (uint32_t)RDC->MR[44].MREA;
    sha512msg[j++] = (uint32_t)RDC->MR[44].MRC;

    sha512msg[j++] = (uint32_t)RDC->MR[56].MRSA;
    sha512msg[j++] = (uint32_t)RDC->MR[56].MREA;
    sha512msg[j++] = (uint32_t)RDC->MR[56].MRC;

    sha512msg[j++] = (uint32_t)RDC->MR[40].MRSA;
    sha512msg[j++] = (uint32_t)RDC->MR[40].MREA;
    sha512msg[j++] = (uint32_t)RDC->MR[40].MRC;

    sha512msg[j++] = (uint32_t)RDC->MR[57].MRSA;
    sha512msg[j++] = (uint32_t)RDC->MR[57].MREA;
    sha512msg[j++] = (uint32_t)RDC->MR[57].MRC;

    sha512msg[j++] = (uint32_t)RDC->MR[48].MRSA;
    sha512msg[j++] = (uint32_t)RDC->MR[48].MREA;
    sha512msg[j++] = (uint32_t)RDC->MR[48].MRC;

    sha512msg[j++] = (uint32_t)RDC->MR[58].MRSA;
    sha512msg[j++] = (uint32_t)RDC->MR[58].MREA;
    sha512msg[j++] = (uint32_t)RDC->MR[58].MRC;

    sha512msg[j++] = (uint32_t)RDC->MR[24].MRSA;
    sha512msg[j++] = (uint32_t)RDC->MR[24].MREA;
    sha512msg[j++] = (uint32_t)RDC->MR[24].MRC;

    sha512msg[j++] = (uint32_t)RDC->MR[32].MRSA;
    sha512msg[j++] = (uint32_t)RDC->MR[32].MREA;
    sha512msg[j++] = (uint32_t)RDC->MR[32].MRC;

    sha512msg[j++] = (uint32_t)RDC->MR[0].MRSA;
    sha512msg[j++] = (uint32_t)RDC->MR[0].MREA;
    sha512msg[j++] = (uint32_t)RDC->MR[0].MRC;

    sha512msg[j++] = (uint32_t)RDC->MR[8].MRSA;
    sha512msg[j++] = (uint32_t)RDC->MR[8].MREA;
    sha512msg[j++] = (uint32_t)RDC->MR[8].MRC;

    sha512msg[j++] = (uint32_t)RDC->MR[16].MRSA;
    sha512msg[j++] = (uint32_t)RDC->MR[16].MREA;
    sha512msg[j++] = (uint32_t)RDC->MR[16].MRC;

    for(i=0; i<128; i++)
    {
    	sha512msg[j++] = (uint32_t)RDC->PDAP[i];
    }

    (void)SE_MbedtlsSha512((uint32_t)sha512msg, sizeof(sha512msg), sha512,  0);

    if(memcmp((void *)cmpSha512, (void *)sha512, sizeof(sha512)) == 0)
    {
    	status = kStatus_SSS_Success;
    }
    else
    {
    	return kStatus_SSS_Fail;
    }

    memset(sha512msg, 0, sizeof(sha512msg));
    memset(sha512, 0, sizeof(sha512));

    j=0;

    for(i=0; i<12; i++)
    {
    	sha512msg[j++] = (uint32_t)RDC->MDA[i];
    }

    sha512msg[j++] = (uint32_t)RDC->MR[44].MRSA;
    sha512msg[j++] = (uint32_t)RDC->MR[44].MREA;
    sha512msg[j++] = (uint32_t)RDC->MR[44].MRC;

    sha512msg[j++] = (uint32_t)RDC->MR[56].MRSA;
    sha512msg[j++] = (uint32_t)RDC->MR[56].MREA;
    sha512msg[j++] = (uint32_t)RDC->MR[56].MRC;

    sha512msg[j++] = (uint32_t)RDC->MR[40].MRSA;
    sha512msg[j++] = (uint32_t)RDC->MR[40].MREA;
    sha512msg[j++] = (uint32_t)RDC->MR[40].MRC;

    sha512msg[j++] = (uint32_t)RDC->MR[57].MRSA;
    sha512msg[j++] = (uint32_t)RDC->MR[57].MREA;
    sha512msg[j++] = (uint32_t)RDC->MR[57].MRC;

    sha512msg[j++] = (uint32_t)RDC->MR[48].MRSA;
    sha512msg[j++] = (uint32_t)RDC->MR[48].MREA;
    sha512msg[j++] = (uint32_t)RDC->MR[48].MRC;

    sha512msg[j++] = (uint32_t)RDC->MR[58].MRSA;
    sha512msg[j++] = (uint32_t)RDC->MR[58].MREA;
    sha512msg[j++] = (uint32_t)RDC->MR[58].MRC;

    sha512msg[j++] = (uint32_t)RDC->MR[24].MRSA;
    sha512msg[j++] = (uint32_t)RDC->MR[24].MREA;
    sha512msg[j++] = (uint32_t)RDC->MR[24].MRC;

    sha512msg[j++] = (uint32_t)RDC->MR[32].MRSA;
    sha512msg[j++] = (uint32_t)RDC->MR[32].MREA;
    sha512msg[j++] = (uint32_t)RDC->MR[32].MRC;

    sha512msg[j++] = (uint32_t)RDC->MR[0].MRSA;
    sha512msg[j++] = (uint32_t)RDC->MR[0].MREA;
    sha512msg[j++] = (uint32_t)RDC->MR[0].MRC;

    sha512msg[j++] = (uint32_t)RDC->MR[8].MRSA;
    sha512msg[j++] = (uint32_t)RDC->MR[8].MREA;
    sha512msg[j++] = (uint32_t)RDC->MR[8].MRC;

    sha512msg[j++] = (uint32_t)RDC->MR[16].MRSA;
    sha512msg[j++] = (uint32_t)RDC->MR[16].MREA;
    sha512msg[j++] = (uint32_t)RDC->MR[16].MRC;

    for(i=0; i<128; i++)
    {
    	sha512msg[j++] = (uint32_t)RDC->PDAP[i];
    }

    (void)SE_MbedtlsSha512((uint32_t)sha512msg, sizeof(sha512msg), sha512,  0);

    if((status == kStatus_SSS_Success) && (memcmp((void *)cmpSha512, (void *)sha512, sizeof(sha512)) == 0))
      return status;

    return  kStatus_SSS_Fail;
}
    
