/*******************************************************************************
*
* Copyright (c) 2013 - 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2018 NXP
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* o Redistributions of source code must retain the above copyright notice, this list
*   of conditions and the following disclaimer.
*
* o Redistributions in binary form must reproduce the above copyright notice, this
*   list of conditions and the following disclaimer in the documentation and/or
*   other materials provided with the distribution.
*
* o Neither the name of the copyright holder nor the names of its
*   contributors may be used to endorse or promote products derived from this
*   software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* 
*
****************************************************************************//*!
*
* @file   mainInclude.js
*
* @brief  Main include file containing all local JSripts files
*
******************************************************************************/

/******************************************************************************
* List of functions
******************************************************************************
* includeJSfiles() 
* build_includeFile_line(jsFileName)
*
*******************************************************************************/
 
/***************************************************************************//*!
*
* @brief   The function includes all required js files
* @param   
* @return  None
* @remarks 
******************************************************************************/
function includeJSfiles()
{
    build_includeFile_line('config.js');
    build_includeFile_line('calculations.js');
    build_includeFile_line('fileProcessing.js');
    build_includeFile_line('hFileConfig.js');
    build_includeFile_line('settings.js');
    build_includeFile_line('formCalculations.js');       
    build_includeFile_line('inner_Parameters.js');
    build_includeFile_line('inner_CLoop.js');
    build_includeFile_line('inner_SLoop.js');
    build_includeFile_line('inner_PoSpeSensor.js');
    build_includeFile_line('inner_PoSpeBemfDQ.js');
    build_includeFile_line('inner_CtrlStruc.js');
    build_includeFile_line('inner_MID.js');
}

/***************************************************************************//*!
*
* @brief   The function build script for including js file
* @param   
* @return  None
* @remarks Function required the same source folder
******************************************************************************/
function build_includeFile_line(jsFileName)
{
  document.write('<scr'+'ipt type="text/javascript" src="JS_functions/' + jsFileName + '" ></scr'+'ipt>');    
}

/***************************************************************************//*!
* 
******************************************************************************
* End of code
******************************************************************************/
 