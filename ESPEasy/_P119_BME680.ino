//#######################################################################################################
//#################### Plugin 120 BME680 I2C Temp/Hum/Barometric/Pressure/Gas Resistence Sensor  ########
//#######################################################################################################

/*******************************************************************************
 * Copyright 2017
 * Written by Rossen Tchobanski (rosko@rosko.net)
 * BSD license, all text above must be included in any redistribution
 *
 * Release notes:
   Adafruit_BME680 Library v1.0.5 required (https://github.com/adafruit/Adafruit_BME680/tree/1.0.5)
*****************************************************************************/


//#ifdef PLUGIN_BUILD_DEV
//#ifdef PLUGIN_BUILD_TESTING

#include <js_BME680.h>

#ifndef PCONFIG
  # define PCONFIG(n) (Settings.TaskDevicePluginConfig[event->TaskIndex][(n)])
#endif // ifndef PCONFIG

#define PLUGIN_119
#define PLUGIN_ID_119         119
#define PLUGIN_NAME_119       "Environment - BME680 - TVOC "
#define PLUGIN_VALUENAME1_119 "Temperature"
#define PLUGIN_VALUENAME2_119 "Humidity"
#define PLUGIN_VALUENAME3_119 "Pressure"
#define PLUGIN_VALUENAME4_119 "TVOC"

#define SEALEVELPRESSURE_HPA (1013.25)

//JS_BME680 is declared in lib !

unsigned long 	cycleCounter  = 0;

unsigned long 	prevTime  = 0;

boolean Plugin_119_init = false;

boolean Plugin_119(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_119;
        Device[deviceCount].Type = DEVICE_TYPE_I2C;
        Device[deviceCount].VType = SENSOR_TYPE_QUAD;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 4;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_119);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_119));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_119));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_119));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[3], PSTR(PLUGIN_VALUENAME4_119));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {        
        byte choice = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        int optionValues[2] = { 0x77, 0x76 };        
        addFormSelectorI2C(F("plugin_119_BME680_i2c"), 2, optionValues, choice);
        addFormNote(F("SDO Low=0x76, High=0x77"));
        
        addFormNumericBox(F("Altitude"), F("plugin_119_BME680_elev"), PCONFIG(1));
        addUnit(F("m"));

        //------------
        addFormSubHeader(F("Measurement options"));
        addFormCheckBox(F("Filter tVOC"), F("plugin_119_BME680_filtered_tvoc_enable"), PCONFIG(2) );

        addFormNumericBox(F("Temp-Offset"), F("plugin_119_BME680_toffs"), PCONFIG(3));
        addUnit(F("°C"));

        addFormNumericBox(F("Hum-Offset"), F("plugin_119_BME680_hoffs"), PCONFIG(4));
        addUnit(F("%"));

        //------------
        addFormSubHeader(F("Debug options"));
        addFormCheckBox(F("Plot output enable"), F("plugin_119_BME680_enable_plot_output"), PCONFIG(5) );
        addFormNote(F("Use with Arduino-Plotter on right COM-Port!"));

        addFormCheckBox(F("Debug output enable"), F("plugin_119_BME680_enable_debug_output"), PCONFIG(6) );
        addFormNote(F("Use Arduino-Serial-Monitor on right COM-Port!"));
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        //--- get int config values
        PCONFIG(0) = getFormItemInt(F("plugin_119_BME680_i2c"));        
        PCONFIG(1) = getFormItemInt(F("plugin_119_BME680_elev"));        
        PCONFIG(3) = getFormItemInt(F("plugin_119_BME680_toffs"));        
        PCONFIG(4) = getFormItemInt(F("plugin_119_BME680_hoffs"));

        //--- save checkboxes
        if (isFormItemChecked( F("plugin_119_BME680_filtered_tvoc_enable")) )
        {
          PCONFIG(2) = 1; 
        }
        else
        {
          PCONFIG(2) = 0;
        }
          
        //--- plot setting 
        if (isFormItemChecked( F("plugin_119_BME680_enable_plot_output")) )
        {
          PCONFIG(5) = 1;
        }
        else
        {         
          PCONFIG(5) = 0;
        } 
        //--- debug         
        if (isFormItemChecked( F("plugin_119_BME680_enable_debug_output")) )
        {
          PCONFIG(6) = 1;
        }
        else
        {
          PCONFIG(6) = 0;
        }
          
        success = true;
        break;
      }

    case PLUGIN_READ:
      {
        if (!Plugin_119_init)
        {
            addLog(LOG_LEVEL_INFO, F("BME680  : init"));

            Plugin_119_init = true; 


            addLog(LOG_LEVEL_INFO, F("BME680  : PLUGIN_READ initialized."));

            String log = F("BME680  : PLUGIN_READ-Settings: I2C: ");
            log += PCONFIG(0) ;
            log += F(" Elevation: ");            
            log += PCONFIG(1);
            log += F(" Filter: ");            
            log += PCONFIG(2);
            log += F(" Toffs: ");            
            log += PCONFIG(3);
            log += F(" Hoffs: ");            
            log += PCONFIG(4);            
            log += F(" Plot: ");            
            log += PCONFIG(5);            
            log += F(" Debug: ");
            log += PCONFIG(6);            
            addLog(LOG_LEVEL_INFO, log);

            //-- checks 
            uint8_t use_this_i2c_adr = 0x76; 
            if (PCONFIG(0) != 0) 
            {
              //--- other than default: 0x77
              use_this_i2c_adr = (uint8_t) PCONFIG(0);             
            }
            bool use_Plotting = false; 
            
            //--- plot-swithed on?
            if (PCONFIG(5) != 0) 
            {
              //--- switch debugging on 
              use_Plotting = (uint8_t) PCONFIG(5);             
            }
            
            //--- debug-swithed on?
            bool use_Debugging = false; 
            if (PCONFIG(6) != 0) 
            {
              //--- switch debugging on 
              use_Debugging = (uint8_t) PCONFIG(6);             
            }

            // --- Temp-offset 
            float t_offs = 0; 
            if (PCONFIG(3) != 0) 
            {
              //--- switch debugging on 
              t_offs = (float) PCONFIG(3);             
            }                           
            
            // --- HUM-offset 
            float h_offs = 0; 
            if (PCONFIG(4) != 0) 
            {
              //--- switch debugging on 
              t_offs = (float) PCONFIG(4);             
            }  
            
            // --- HUM-offset 
            bool use_filtered_tvoc = false; 
            if (PCONFIG(2) != 0) 
            {
              //--- switch filtered output on 
              use_filtered_tvoc = true;             
            }  

            // --- setup code,  run once
			      JS_BME680.set_bme680_device_address(use_this_i2c_adr);  // may be ommitted in case of default-address 0x76 (SDO = GND), declare in case of address 0x77 (SDO = High)      
            JS_BME680.useArduinoDebugOutput    = use_Debugging; 
            JS_BME680.useArduinoPlotOutput     = use_Plotting;
            JS_BME680.set_bme680_offset_hum ( h_offs); 
            JS_BME680.set_bme680_offset_temp (t_offs);
      
            //--- start measuring 
            JS_BME680.do_begin(); 

			      addLog(LOG_LEVEL_INFO, F("JS_BME680 : ready + initialized!"));
			
            success = true;
            break;
        }
/*
        if (Plugin_119_init)
        {
            if (! JS_BME680.do_bme680_measurement()) {
              addLog(LOG_LEVEL_ERROR, F("BME680 : Failed to perform reading!"));
              success = false;
              break;
            }
*/ 
			
            //--- put your main code here, to run repeatedly:
            static unsigned long baseIntervall = JS_BME680.get_bme680Interval(); 
            unsigned long currentMillis = millis();
          
            //---bme680 data screen added
            // if (currentMillis - prevTime > baseIntervall) 
            // {   
              // --- ca. alle 10 Sekunden eine Messung aller Messgroessen      
              //JS_BME680.do_bme680_measurement();
            //   prevTime = currentMillis; 
            // }
          

            UserVar[event->BaseVarIndex + 0] = JS_BME680.getTemp(); 
            UserVar[event->BaseVarIndex + 1] = JS_BME680.getHum();
            UserVar[event->BaseVarIndex + 2] = JS_BME680.getPress();
            UserVar[event->BaseVarIndex + 3] = JS_BME680.getTVoc();

        //}

        success = true;
        break;
      }

      case PLUGIN_ONCE_A_SECOND:
      {
        //code to be executed once a second. Tasks which do not require fast response can be added here
        cycleCounter ++; 
        if (cycleCounter >= 10)
        {
          JS_BME680.do_bme680_measurement();
          cycleCounter = 0; 
        }
        success = true;

      }

  }
  return success;
}

//#endif