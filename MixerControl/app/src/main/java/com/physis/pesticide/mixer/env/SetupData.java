package com.physis.pesticide.mixer.env;

public class SetupData {
    private String powderMotorProceed, powderWaterSpray;
    private String pesticideMotorProceed, pesticideWaterSpray;
    private String agitatorTime, airPumpTime;

    private static SetupData setupData = null;

    private SetupData(){

    }

    public synchronized static SetupData getInstance(){
        if(setupData == null)
            setupData = new SetupData();
        return setupData;
    }

    public void setSetupData(String info){
        String[] infos = info.split(",");
        this.powderMotorProceed = infos[0];
        this.powderWaterSpray = infos[1];
        this.pesticideMotorProceed = infos[2];
        this.pesticideWaterSpray = infos[3];
        this.airPumpTime = infos[4];
        this.agitatorTime = infos[5];
    }

    public String getPesticideMotorProceed() {
        return pesticideMotorProceed;
    }

    public String getPesticideWaterSpray() {
        return pesticideWaterSpray;
    }

    public String getPowderMotorProceed() {
        return powderMotorProceed;
    }

    public String getPowderWaterSpray() {
        return powderWaterSpray;
    }

    public String getAgitatorTime() {
        return agitatorTime;
    }

    public String getAirPumpTime() {
        return airPumpTime;
    }
}
