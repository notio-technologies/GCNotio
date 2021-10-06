package com.ridelogger.listners;
import android.preference.PreferenceManager;

import com.dsi.ant.plugins.antplus.pcc.AntPlusBikePowerPcc;
import com.dsi.ant.plugins.antplus.pcc.AntPlusBikePowerPcc.CalculatedWheelDistanceReceiver;
import com.dsi.ant.plugins.antplus.pcc.AntPlusBikePowerPcc.CalculatedWheelSpeedReceiver;
import com.dsi.ant.plugins.antplus.pcc.AntPlusBikePowerPcc.DataSource;
import com.dsi.ant.plugins.antplus.pcc.AntPlusBikePowerPcc.ICalculatedCrankCadenceReceiver;
import com.dsi.ant.plugins.antplus.pcc.AntPlusBikePowerPcc.ICalculatedPowerReceiver;
import com.dsi.ant.plugins.antplus.pcc.AntPlusBikePowerPcc.ICalculatedTorqueReceiver;
import com.dsi.ant.plugins.antplus.pcc.AntPlusBikePowerPcc.IInstantaneousCadenceReceiver;
import com.dsi.ant.plugins.antplus.pcc.AntPlusBikePowerPcc.IRawPowerOnlyDataReceiver;
import com.dsi.ant.plugins.antplus.pcc.defines.DeviceState;
import com.dsi.ant.plugins.antplus.pcc.defines.EventFlag;
import com.dsi.ant.plugins.antplus.pcc.defines.RequestAccessResult;
import com.dsi.ant.plugins.antplus.pccbase.AntPluginPcc.IPluginAccessResultReceiver;
import com.ridelogger.R;
import com.ridelogger.RideService;

import java.math.BigDecimal;
import java.util.EnumSet;

/**
 * Power
 * @author Chet Henry
 * Listen to and log Ant+ Power events
 */
public class Power extends Ant
{
    public BigDecimal wheelCircumferenceInMeters; //size of wheel to calculate speed
    public IPluginAccessResultReceiver<AntPlusBikePowerPcc> mResultReceiver;
    
    //setup listeners and logging 
    public Power(int pDeviceNumber, RideService mContext) {
        super(pDeviceNumber, mContext);
        
        wheelCircumferenceInMeters = new BigDecimal(
                PreferenceManager.getDefaultSharedPreferences(context).getString(context.getString(R.string.PREF_WHEEL_SIZE), "2.096")
        );
        
        //Handle messages
        mResultReceiver = new IPluginAccessResultReceiver<AntPlusBikePowerPcc>() {
           //Handle the result, connecting to events on success or reporting failure to user.
           @Override
           public void onResultReceived(AntPlusBikePowerPcc result, RequestAccessResult resultCode, DeviceState initialDeviceState) {
               if(resultCode == com.dsi.ant.plugins.antplus.pcc.defines.RequestAccessResult.SUCCESS) {
                   deviceNumber = result.getAntDeviceNumber();
                   
                   result.subscribeCalculatedPowerEvent(new ICalculatedPowerReceiver() {
                           @Override
                           public void onNewCalculatedPower(final long estTimestamp, final EnumSet<EventFlag> eventFlags, final DataSource dataSource, final BigDecimal calculatedPower) {
                               alterCurrentData(RideService.WATTS, calculatedPower.floatValue());
                           }
                       }
                   );

                   result.subscribeCalculatedTorqueEvent(
                       new ICalculatedTorqueReceiver() {
                           @Override
                           public void onNewCalculatedTorque(final long estTimestamp, final EnumSet<EventFlag> eventFlags, final DataSource dataSource, final BigDecimal calculatedTorque) {
                               alterCurrentData(RideService.NM, calculatedTorque.floatValue());
                           }
                       }
                   );

                   result.subscribeCalculatedCrankCadenceEvent(
                       new ICalculatedCrankCadenceReceiver() {
                           @Override
                           public void onNewCalculatedCrankCadence(final long estTimestamp, final EnumSet<EventFlag> eventFlags, final DataSource dataSource, final BigDecimal calculatedCrankCadence) {
                               alterCurrentData(RideService.CAD, calculatedCrankCadence.floatValue());
                           }
                       }
                   );

                   result.subscribeCalculatedWheelSpeedEvent(
                       new CalculatedWheelSpeedReceiver(wheelCircumferenceInMeters) {
                           @Override
                           public void onNewCalculatedWheelSpeed(final long estTimestamp, final EnumSet<EventFlag> eventFlags, final DataSource dataSource, final BigDecimal calculatedWheelSpeed)
                           {
                               alterCurrentData(RideService.KPH, calculatedWheelSpeed.floatValue());
                           }
                       }
                   );

                   result.subscribeCalculatedWheelDistanceEvent(
                       new CalculatedWheelDistanceReceiver(wheelCircumferenceInMeters) {
                           @Override
                           public void onNewCalculatedWheelDistance(final long estTimestamp, final EnumSet<EventFlag> eventFlags, final DataSource dataSource, final BigDecimal calculatedWheelDistance) 
                           {
                               alterCurrentData(RideService.KM, calculatedWheelDistance.floatValue());
                           }
                       }
                   );

                   result.subscribeInstantaneousCadenceEvent(
                       new IInstantaneousCadenceReceiver() {
                           @Override
                           public void onNewInstantaneousCadence(final long estTimestamp, final EnumSet<EventFlag> eventFlags, final DataSource dataSource, final int instantaneousCadence)
                           {
                               alterCurrentData(RideService.CAD, (float) instantaneousCadence);
                           }
                       }
                   );

                   result.subscribeRawPowerOnlyDataEvent(
                       new IRawPowerOnlyDataReceiver() {
                           @Override
                           public void onNewRawPowerOnlyData(final long estTimestamp, final EnumSet<EventFlag> eventFlags, final long powerOnlyUpdateEventCount, final int instantaneousPower, final long accumulatedPower)
                           {
                               alterCurrentData(RideService.WATTS, (float) instantaneousPower);
                           }
                       }
                   );

                   /*result.subscribePedalPowerBalanceEvent(
                       new IPedalPowerBalanceReceiver() {
                           @Override
                           public void onNewPedalPowerBalance(final long estTimestamp, final EnumSet<EventFlag> eventFlags, final boolean rightPedalIndicator, final int pedalPowerPercentage)
                           {
                               alterCurrentData(RideService.LTE, pedalPowerPercentage);
                           }
                       }
                   );

                   result.subscribeRawWheelTorqueDataEvent(
                       new IRawWheelTorqueDataReceiver() {
                           @Override
                           public void onNewRawWheelTorqueData(final long estTimestamp, final EnumSet<EventFlag> eventFlags, final long wheelTorqueUpdateEventCount, final long accumulatedWheelTicks, final BigDecimal accumulatedWheelPeriod, final BigDecimal accumulatedWheelTorque)
                           {
                               alterCurrentData(RideService.NM, accumulatedWheelTorque);
                           }
                       }
                   );

                   result.subscribeRawCrankTorqueDataEvent(
                       new IRawCrankTorqueDataReceiver() {
                           @Override
                           public void onNewRawCrankTorqueData(final long estTimestamp, final EnumSet<EventFlag> eventFlags, final long crankTorqueUpdateEventCount, final long accumulatedCrankTicks, final BigDecimal accumulatedCrankPeriod, final BigDecimal accumulatedCrankTorque)
                           {
                               alterCurrentData(RideService.NM, accumulatedCrankTorque);
                           }
                       }
                   );

                   result.subscribeTorqueEffectivenessEvent(
                       new ITorqueEffectivenessReceiver() {
                           @Override
                           public void onNewTorqueEffectiveness(final long estTimestamp, final EnumSet<EventFlag> eventFlags, final long powerOnlyUpdateEventCount, final BigDecimal leftTorqueEffectiveness, final BigDecimal rightTorqueEffectiveness)
                           {                            
                               int[] keys = {
                                       RideService.LTE,
                                       RideService.RTE
                               };
                               
                               float[] values = {
                                       leftTorqueEffectiveness,
                                       rightTorqueEffectiveness
                               }
                               
                               alterCurrentData(keys, values);
                           }
       
                       }
                   );

                   result.subscribePedalSmoothnessEvent(new IPedalSmoothnessReceiver() {
                           @Override
                           public void onNewPedalSmoothness(final long estTimestamp, final EnumSet<EventFlag> eventFlags, final long powerOnlyUpdateEventCount, final boolean separatePedalSmoothnessSupport, final BigDecimal leftOrCombinedPedalSmoothness, final BigDecimal rightPedalSmoothness)
                           {
                               int[] keys = {
                                       RideService.SNPLC,
                                       RideService.SNPR
                               };
                               
                               float[] values = {
                                       leftOrCombinedPedalSmoothness,
                                       rightPedalSmoothness
                               }

                               alterCurrentData(map);
                           }
                       }
                   );*/
               } else if(resultCode == com.dsi.ant.plugins.antplus.pcc.defines.RequestAccessResult.SEARCH_TIMEOUT) {                   
                   if(deviceNumber != 0) {
                       requestAccess();
                   }
               }
           }
       };
       
       requestAccess();
    }
    
    protected void requestAccess() {
        releaseHandle = AntPlusBikePowerPcc.requestAccess(context, deviceNumber, 0, mResultReceiver, mDeviceStateChangeReceiver);
    }
    
    
    @Override
    public void zeroReadings()
    {   
        int[] keys = {
                RideService.WATTS,
                RideService.NM, 
                RideService.CAD,
                RideService.KPH,
                RideService.KM
        };
        
        float[] values = {
                (float) 0.0,
                (float) 0.0,
                (float) 0.0,
                (float) 0.0,
                (float) 0.0
        };
        
        alterCurrentData(keys, values);
    }
}


