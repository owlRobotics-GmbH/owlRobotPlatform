
#define TimeWD 1500

class cmd {
  public:
  cmd();
  void cmdM();
  void cmdV();
  void cmdS();
  void cmdJ();   
  void cmdB(); 
  void cmdP();  
  void NOcmd();
  int atLeftSpeed = 0;
  int atRightSpeed = 0;
  int atSpraySpeed = 0;
  int atPressureEnable = 0;
  int atAllowManualCtl = 1;
  float pidP = 0;
  float pidI = 0;
  float pidD = 0;
  private:
  String ans;

};
