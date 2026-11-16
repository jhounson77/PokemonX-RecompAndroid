package org.pokemonx.recomp;
import android.app.*;
import android.content.*;
import android.database.Cursor;
import android.net.Uri;
import android.os.*;
import android.provider.OpenableColumns;
import android.view.*;
import android.widget.*;
import android.graphics.Color;
import java.io.*;
import java.util.*;
import java.util.concurrent.atomic.AtomicBoolean;
public final class MainActivity extends Activity implements SurfaceHolder.Callback {
 static {System.loadLibrary("pxhost");}
 static native long[] nativeCounters();
 static native double[] nativePerformance();
 private static native void nativeSurface(Surface s);
 private static native void nativeMode(int mode);
 static native void nativeMoveStats(int[] output);
 private static native void nativeTextures(boolean enabled);
 static native void nativeBattleRegions(int width,int height,float[] output);
 static native void nativeCommitUI(int profile,int index);
 private static native void nativeOpenPopup();
 private static native boolean nativePopupVisible();
 private static native int nativeEffectiveMode();
 private static native void nativeAutomaticUI(boolean enabled);
 private static native void nativeMoveCards(boolean enabled);
 private static native void nativeBattleMetadata(boolean enabled);
 static native int[] nativeMapTouch(float x,float y,int width,int height,int mode);
 private static native void nativeRenderer(int mode);
 private static native void nativeFrameTarget(int fps);
 private static native void nativeConfigure(int internalScale,String driverDirectory,String driverLibrary);
 private static native void nativeInput(int buttons,int cx,int cy,int tx,int ty);
 private static native void nativePause(boolean pause);
 private static native void nativeStop();
 private static native boolean nativePrepare();
 private native void nativeRun(String directory,String libraries,String sessionLog,boolean warming);
 private final AtomicBoolean running=new AtomicBoolean(false);
 private PerformanceHud performanceHud;
 private void stopHud(){if(performanceHud!=null){performanceHud.stop();performanceHud=null;}}
 private SharedPreferences settings;private SurfaceView screen;private TextView status;private String page="launcher",message="";
 private boolean preparingShaders=false;
 private boolean importing=false,surfaceReady=false,active=true,pausedMenu=false,pendingStart=false;
 private View controlOverlay;private boolean controlsVisible=true,interfaceDialogOpen=false;private boolean xAsBack=false;private int screenPointer=-1,screenTouchMode=-1,lastInputMask=0,popupReturnMode=0;
 private int leftMask=0,rightMask=0,circleX=0,circleY=0,touchX=-1,touchY=-1,screenMode=0;
 private int dp(int n){return Math.round(n*getResources().getDisplayMetrics().density);}
 private SessionLogs sessionLogs;private File launcherLog;private volatile File sessionLog;
 private File log(){return sessionLog!=null?sessionLog:launcherLog;}
 private int versionCode(){try{return getPackageManager().getPackageInfo(getPackageName(),0).versionCode;}catch(Exception ignored){return 52;}}
 private String logHeader(){return "APK versionCode="+versionCode()+"; Android "+Build.VERSION.RELEASE+" API "+Build.VERSION.SDK_INT+"; "+Build.MANUFACTURER+" "+Build.MODEL+"; PID="+android.os.Process.myPid();}
 private boolean hasGame(){return new File(getFilesDir(),"game.cxi").isFile();}
 private void append(String m){File target=running.get()&&sessionLog!=null?sessionLog:launcherLog;if(target!=null)try{SessionLogs.append(target,m);}catch(IOException error){android.util.Log.e("PokemonX","Cannot write session log",error);}}
 private void toast(String m){Toast.makeText(this,m,Toast.LENGTH_LONG).show();}
 @Override public void onCreate(Bundle state){super.onCreate(state);settings=getSharedPreferences("settings",0);I18n.setLanguage(settings.getString("app_language","pt-BR"));getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);getWindow().getDecorView().setSystemUiVisibility(5894);
  try{sessionLogs=new SessionLogs(getFilesDir());launcherLog=sessionLogs.begin(false,versionCode(),System.currentTimeMillis(),logHeader());sessionLog=sessionLogs.restore(settings.getString("last_session_log",null));}catch(IOException error){message=I18n.t("Falha ao criar logs: ")+I18n.runtime(error.getMessage()==null?error.getClass().getSimpleName():error.getMessage());}
  append("Build V0.1.2 Teste 17 launcher opened");
  Thread.UncaughtExceptionHandler previous=Thread.getDefaultUncaughtExceptionHandler();Thread.setDefaultUncaughtExceptionHandler((t,e)->{append(android.util.Log.getStackTraceString(e));if(previous!=null)previous.uncaughtException(t,e);});importing=true;showLauncher();new Thread(()->{try{TextureMods.list(getFilesDir());}catch(Exception error){message=I18n.t("Falha: ")+I18n.runtime(error.getMessage());append("Mod recovery failed: "+error);}finally{importing=false;runOnUiThread(this::showLauncher);}},"ModRecovery").start();
 }
 private LinearLayout column(){LinearLayout l=new LinearLayout(this);l.setOrientation(LinearLayout.VERTICAL);return l;}
 private TextView text(String value,int size,int color){TextView t=new TextView(this);t.setText(value);t.setTextSize(size);t.setTextColor(color);t.setPadding(dp(4),dp(4),dp(4),dp(4));return t;}
 private Button button(String label,Runnable action){Button b=new Button(this);b.setText(label);b.setTextSize(13);b.setAllCaps(false);b.setTextColor(Color.WHITE);b.setBackgroundTintList(android.content.res.ColorStateList.valueOf(0xff243b56));b.setOnClickListener(v->action.run());return b;}
 private void addButton(LinearLayout parent,String label,Runnable action){Button b=button(label,action);LinearLayout.LayoutParams p=new LinearLayout.LayoutParams(-1,dp(47));p.setMargins(0,dp(3),0,dp(3));parent.addView(b,p);if(importing||running.get())b.setEnabled(label.equals(I18n.t("Exportar logs")));}
 private void showLauncher(){stopHud();page="launcher";pendingStart=false;screen=null;surfaceReady=false;nativePause(true);nativeSurface(null);releaseInput();
  LauncherView home=new LauncherView(this,hasGame(),importing||running.get(),importing?message:running.get()?I18n.t("Encerrando e salvando…"):"", "V0.1.2 Teste 17",
   ()->{if(importing||running.get())return;if(!hasGame()){message=I18n.t("Importe seu arquivo CIA para começar.");showSettings();return;}showGame(true);},
   this::showSettings,()->{if(importing){toast(I18n.t("Aguarde o fim da importação."));return;}nativeStop();finish();});
  status=home.statusView();setContentView(home);
 }
 private int settingsSection=0;
 private void showSettings(){stopHud();page="settings";pendingStart=false;screen=null;surfaceReady=false;nativePause(true);nativeSurface(null);releaseInput();
  boolean busy=importing||running.get();SettingsView ui=new SettingsView(this,settingsSection,busy,message,this::showLauncher,index->{settingsSection=index;showSettings();});status=ui.status;
  if(settingsSection==0){
   ui.heading(I18n.t("Jogo e arquivos"),I18n.t("Importação e atualização do seu Pokémon X."));
   ui.action(hasGame()?I18n.t("Substituir jogo base"):I18n.t("Importar jogo base"),hasGame()?I18n.t("CIA importado • saves preservados"):I18n.t("Selecione seu arquivo CIA"),!busy,()->pick(7));
   ui.action(UpdateInstaller.installed(this)?I18n.t("Reinstalar atualização 1.5"):I18n.t("Instalar atualização 1.5"),I18n.t("Aceita CIA ou RAR"),!busy&&hasGame(),()->pick(9));
   if(UpdateInstaller.installed(this))ui.toggle(I18n.t("Usar atualização 1.5"),I18n.t("Desative para voltar à versão base. Os saves são mantidos."),UpdateInstaller.enabled(this),on->{try{UpdateInstaller.setEnabled(UpdateInstaller.root(this),on);message=on?I18n.t("Atualização 1.5 ativada."):I18n.t("Versão base ativada.");}catch(Exception error){message=I18n.t("Falha: ")+I18n.runtime(error.getMessage()==null?error.getClass().getSimpleName():error.getMessage());}showSettings();});
   ui.action(I18n.t("Adicionar mod de texturas"),I18n.t("Pack PNG Citra/Azahar em ZIP • mantém os outros mods"),!busy,()->pick(10));
   ui.note(I18n.t("Vários packs podem ficar ativos. Em conflitos, vence o de maior prioridade; novos packs entram no topo."));
   ui.action(I18n.t("Gerenciar mods de texturas"),I18n.t("Ativar, desativar, remover e escolher prioridade"),!busy,this::manageMods);
   if(ModInstaller.installed(getFilesDir()))ui.toggle(I18n.t("Ativar mods de texturas"),I18n.t("Aplicado na próxima sessão. Texturas maiores podem usar mais RAM."),settings.getBoolean("texture_mod",true),on->settings.edit().putBoolean("texture_mod",on).apply());
   ui.action(I18n.t("Exportar save"),I18n.t("Salve no menu do jogo antes de exportar"),!busy,()->saveDestination(12));
   ui.action(I18n.t("Importar save"),I18n.t("Backup ZIP ou arquivo main • cria backup do save atual"),!busy,()->pick(11));
   if(SaveManager.latestBackup(getFilesDir())!=null)ui.action(I18n.t("Exportar backup anterior"),I18n.t("Recupere o save preservado antes da última importação"),!busy,()->saveDestination(13));
   ui.note(UpdateInstaller.enabled(this)?I18n.t("Versão ativa: atualização 1.5"):I18n.t("Versão ativa: jogo base"));
   ui.toggle(I18n.t("Preparar shaders após importar"),I18n.t("Reutiliza os shaders já conhecidos pelo jogo."),settings.getBoolean("warm_after_import",true),on->settings.edit().putBoolean("warm_after_import",on).apply());
   ui.action(I18n.t("Preparar shaders conhecidos"),I18n.t("Novos efeitos são compilados e armazenados durante o jogo."),!busy&&hasGame(),this::prepareShaders);
  }else if(settingsSection==1){
   ui.heading(I18n.t("Imagem e telas"),I18n.t("Renderização Vulkan · alterações salvas automaticamente."));
   ui.choice(I18n.t("Modo de jogo"),new String[]{I18n.t("Uma tela · interface integrada"),I18n.t("Duas telas · lado a lado")},settings.getInt("screen_layout",0),v->{settings.edit().putInt("screen_layout",v).apply();showSettings();});
   ui.choice(I18n.t("Limite de quadros na tela"),new String[]{I18n.t("30 FPS"),I18n.t("60 FPS · interpolação experimental")},settings.getInt("frame_target",30)==60?1:0,v->{settings.edit().putInt("frame_target",v==1?60:30).apply();showSettings();});
   ui.note(I18n.t("60 FPS cria quadros intermediários sem acelerar a lógica. Pode causar artefatos e atraso visual. Em cenas pesadas, reduz a interpolação. Aplicado na próxima sessão."));
   int scale=settings.getInt("render_scale",1);ui.choice(I18n.t("Resolução de renderização"),new String[]{"1× · 400 × 240","2× · 800 × 480","3× · 1200 × 720","4× · 1600 × 960",I18n.t("Automática · até 4×")},scale==0?4:scale-1,v->{settings.edit().putInt("render_scale",v==4?0:v+1).remove("output_height").apply();showSettings();});
   ui.note(I18n.t("Escala interna real, mantendo a proporção do 3DS. Resoluções maiores exigem mais da GPU."));
   List<DriverManager.Driver> available=DriverManager.list(this);String[] labels=new String[available.size()+1];labels[0]=I18n.t("Sistema (padrão)");int selected=0;String current=settings.getString("driver_path","");for(int i=0;i<available.size();i++){labels[i+1]=available.get(i).name;if(current.equals(available.get(i).dir.getAbsolutePath()))selected=i+1;}
   ui.choice(I18n.t("Driver Vulkan"),labels,selected,v->{settings.edit().putString("driver_path",v==0?"":available.get(v-1).dir.getAbsolutePath()).apply();showSettings();});
   ui.action(I18n.t("Importar driver"),I18n.t("Selecione um pacote ZIP compatível"),!busy,()->pick(8));
  }else if(settingsSection==2){
   ui.heading(I18n.t("Controles e interface"),I18n.t("Personalize a forma de jogar."));
   ui.toggle(I18n.t("Controles sobre o jogo"),I18n.t("Mantém os botões touch sobre a imagem."),settings.getBoolean("overlay_controls",true),on->settings.edit().putBoolean("overlay_controls",on).apply());
   ui.action(I18n.t("Editar controles"),I18n.t("Arraste os botões e ajuste o tamanho"),!busy,this::showEditor);
   ui.toggle(I18n.t("Pokémon e golpes — experimental"),I18n.t("Atualização 1.5: dados após usar um golpe. Trocas forçadas ainda em teste."),settings.getBoolean("battle_metadata_test",false),on->settings.edit().putBoolean("battle_metadata_test",on).apply());
   ui.toggle(I18n.t("Interface automática de batalha"),I18n.t("Em uma tela, mostra as opções apenas durante a seleção."),settings.getBoolean("automatic_battle_ui",true),on->settings.edit().putBoolean("automatic_battle_ui",on).apply());
   ui.toggle(I18n.t("Golpes em cartões"),I18n.t("Nomes e detalhes atualizados pelo próprio jogo."),settings.getBoolean("move_cards",true),on->settings.edit().putBoolean("move_cards",on).apply());
   ui.note(I18n.t("X abre o menu original no canto inferior direito. Os controles touch permanecem visíveis durante as batalhas."));
  }else{
   ui.heading(I18n.t("Sistema"),I18n.t("Diagnóstico e informações do aplicativo."));
   ui.choice(I18n.t("Idioma do aplicativo"),new String[]{"Português (Brasil)","English","Español","Français"},I18n.languageIndex(),index->{String tag=I18n.TAGS[index];settings.edit().putString("app_language",tag).apply();I18n.setLanguage(tag);message="";showSettings();});
   ui.note(I18n.t("O idioma do jogo é independente desta opção."));
   ui.toggle(I18n.t("Indicadores de desempenho"),I18n.t("FPS, velocidade, RAM do app e temperatura da bateria."),settings.getBoolean("performance_hud",true),on->settings.edit().putBoolean("performance_hud",on).apply());
   ui.action(I18n.t("Exportar logs"),I18n.t("Compartilhe o registro da última sessão"),true,this::exportLog);
   ui.note(I18n.t("Pokémon X Recomp · V0.1.2 Teste 17\nNintendo 3DS · Vulkan · Áudio estéreo"));
  }setContentView(ui);
 }

 private void modTask(Runnable task){if(importing||running.get())return;importing=true;message=I18n.t("Aplicando mods…");showSettings();new Thread(()->{task.run();importing=false;runOnUiThread(this::showSettings);},"TextureMods").start();}
 private void manageMods(){if(importing||running.get())return;try{
  List<TextureMods.Pack> packs=TextureMods.list(getFilesDir());Collections.reverse(packs);String[] labels=new String[packs.size()];for(int i=0;i<packs.size();i++){TextureMods.Pack p=packs.get(i);labels[i]=(i+1)+". "+p.name+" · "+I18n.t(p.enabled?"Ativo":"Desativado")+" · "+p.count;}
  AlertDialog.Builder dialog=new AlertDialog.Builder(this).setTitle(I18n.t("Mods • maior prioridade primeiro")).setNegativeButton(I18n.t("Fechar"),null);
  if(packs.isEmpty())dialog.setMessage(I18n.t("Nenhum mod instalado."));else dialog.setItems(labels,(d,i)->modOptions(packs.get(i)));dialog.show();
 }catch(Exception error){message=I18n.t("Falha: ")+I18n.runtime(error.getMessage());showSettings();}}
 private void modOptions(TextureMods.Pack p){new AlertDialog.Builder(this).setTitle(p.name).setItems(new String[]{I18n.t(p.enabled?"Desativar":"Ativar"),I18n.t("Dar maior prioridade"),I18n.t("Remover mod")},(d,action)->{
  Runnable change=()->modTask(()->{try{TextureMods.change(getFilesDir(),p.id,action);message=I18n.t("Mods atualizados. Aplicados na próxima sessão.");}catch(Exception error){message=I18n.t("Falha: ")+I18n.runtime(error.getMessage());}});
  if(action==2)new AlertDialog.Builder(this).setTitle(I18n.t("Remover mod")).setMessage(p.name).setNegativeButton(I18n.t("Cancelar"),null).setPositiveButton(I18n.t("Remover mod"),(a,b)->change.run()).show();else change.run();
 }).setNegativeButton(I18n.t("Cancelar"),null).show();}

 private void applyConfiguration(){nativeFrameTarget(settings.getInt("frame_target",30));nativeBattleMetadata(settings.getBoolean("battle_metadata_test",false));nativeTextures(ModInstaller.installed(getFilesDir())&&settings.getBoolean("texture_mod",true));nativeMoveCards(settings.getBoolean("move_cards",true));nativeAutomaticUI(settings.getBoolean("automatic_battle_ui",true));int renderer=0;nativeRenderer(renderer);String path=settings.getString("driver_path",""),library="";
  if(!path.isEmpty())try{DriverManager.Driver d=DriverManager.read(new File(path));library=d.library;}catch(Exception e){append("Driver selection invalid: "+e);path="";settings.edit().remove("driver_path").apply();}
  nativeConfigure(settings.getInt("render_scale",1),path,library);
 }
 private View compactMenu(){View icon=new View(this){final android.graphics.Paint paint=new android.graphics.Paint(android.graphics.Paint.ANTI_ALIAS_FLAG);protected void onDraw(android.graphics.Canvas c){float cx=getWidth()/2f,cy=getHeight()/2f,r=getWidth()*.34f;paint.setStyle(android.graphics.Paint.Style.FILL);paint.setColor(0x08000000);c.drawCircle(cx,cy,r,paint);paint.setStyle(android.graphics.Paint.Style.STROKE);paint.setStrokeWidth(getResources().getDisplayMetrics().density);paint.setColor(0x88ffffff);c.drawCircle(cx,cy,r,paint);paint.setStyle(android.graphics.Paint.Style.FILL);paint.setColor(0xc8ffffff);for(int i=-1;i<=1;i++)c.drawCircle(cx,cy+i*getWidth()*.11f,getWidth()*.026f,paint);}};icon.setContentDescription(I18n.t("Menu do jogo"));icon.setOnClickListener(v->gameMenu());return icon;}
 private void showGame(boolean start){preparingShaders=false;stopHud();page="game";pausedMenu=false;pendingStart=start;screenMode=settings.getInt("screen_layout",0)==1?1:0;nativeMode(screenMode);controlsVisible=true;lastInputMask=0;popupReturnMode=0;controlOverlay=null;applyConfiguration();releaseInput();
  boolean overlay=settings.getBoolean("overlay_controls",true);FrameLayout root=new FrameLayout(this);root.setBackgroundColor(Color.BLACK);
  ControlPad left=new ControlPad(this,true,false,overlay,(mask,x,y)->{leftMask=mask;circleX=x;circleY=y;sendInput();}),right=new ControlPad(this,false,false,overlay,(mask,x,y)->{rightMask=mask;sendInput();});
  screen=new SurfaceView(this);screen.getHolder().addCallback(this);screen.setOnTouchListener((v,e)->gameTouch(e));
  if(overlay){root.addView(screen,new FrameLayout.LayoutParams(-1,-1));LinearLayout pads=new LinearLayout(this);pads.setOrientation(LinearLayout.HORIZONTAL);pads.addView(left,new LinearLayout.LayoutParams(0,-1,1));pads.addView(right,new LinearLayout.LayoutParams(0,-1,1));root.addView(pads,new FrameLayout.LayoutParams(-1,-1));controlOverlay=pads;}
  else{LinearLayout game=new LinearLayout(this);int rail=Math.min(dp(116),getResources().getDisplayMetrics().widthPixels/5);game.addView(left,new LinearLayout.LayoutParams(rail,-1));game.addView(screen,new LinearLayout.LayoutParams(0,-1,1));game.addView(right,new LinearLayout.LayoutParams(rail,-1));root.addView(game,new FrameLayout.LayoutParams(-1,-1));}
  FrameLayout.LayoutParams menuPosition=new FrameLayout.LayoutParams(dp(44),dp(44),Gravity.TOP|Gravity.CENTER_HORIZONTAL);menuPosition.topMargin=dp(10);root.addView(compactMenu(),menuPosition);
  root.addView(new BattlePanel(this,screen),new FrameLayout.LayoutParams(-1,-1));
  if(settings.getBoolean("performance_hud",true)){TextView hud=new TextView(this);hud.setTextSize(11);hud.setTextColor(0xcfffffff);hud.setShadowLayer(dp(1),0,dp(1),Color.BLACK);hud.setPadding(dp(3),dp(2),dp(3),dp(2));hud.setText(I18n.t("FPS —  ·  Vel. —%\nRAM — MB  ·  Bat. — °C"));hud.setClickable(false);hud.setFocusable(false);FrameLayout.LayoutParams hp=new FrameLayout.LayoutParams(-2,-2,Gravity.TOP|Gravity.LEFT);hp.leftMargin=dp(6);hp.topMargin=dp(57);root.addView(hud,hp);performanceHud=new PerformanceHud(this,hud);if(active)performanceHud.start();}
  setContentView(root);nativePause(false);
 }
 private void prepareShaders(){
  if(!hasGame()||importing||running.get())return;
  stopHud();preparingShaders=true;page="shaders";pausedMenu=false;pendingStart=true;nativeMode(0);applyConfiguration();releaseInput();
  FrameLayout root=new FrameLayout(this);root.setBackgroundColor(0xff101a24);
  screen=new SurfaceView(this);screen.getHolder().addCallback(this);root.addView(screen,new FrameLayout.LayoutParams(-1,-1));
  status=text(I18n.t("Preparando shaders conhecidos…\nNa primeira importação o cache ainda pode estar vazio.\nNovos efeitos serão compilados e guardados ao jogar."),16,Color.WHITE);status.setGravity(Gravity.CENTER);status.setBackgroundColor(0xff101a24);root.addView(status,new FrameLayout.LayoutParams(-1,-1));setContentView(root);nativePause(false);
 }
 private boolean gameTouch(MotionEvent event){
  int action=event.getActionMasked();
  if(action==MotionEvent.ACTION_DOWN){screenPointer=event.getPointerId(event.getActionIndex());screenTouchMode=nativeEffectiveMode();if(screenTouchMode==0)screenTouchMode=-1;}
  if(action==MotionEvent.ACTION_UP||action==MotionEvent.ACTION_CANCEL||(action==MotionEvent.ACTION_POINTER_UP&&event.getPointerId(event.getActionIndex())==screenPointer)){screenPointer=-1;touchX=touchY=-1;sendInput();return true;}
  int index=event.findPointerIndex(screenPointer);
  if(index<0||screen==null){touchX=touchY=-1;sendInput();return true;}
  int[] point=nativeMapTouch(event.getX(index),event.getY(index),screen.getWidth(),screen.getHeight(),screenTouchMode);
  touchX=point==null?-1:point[0];touchY=point==null?-1:point[1];sendInput();return true;
 }
 private void setScreenProfile(int value){screenMode=value;screenPointer=-1;releaseInput();if(value==5)nativeOpenPopup();else nativeMode(value);
  settings.edit().putInt("screen_layout",value==1?1:0).apply();
  if(controlOverlay!=null)controlOverlay.setVisibility(controlsVisible?View.VISIBLE:View.GONE);
  append("Screen profile selected="+value+"; manual UI scene; controls="+controlsVisible);
 }
 private void screenOptions(){interfaceDialogOpen=true;
  String[] choices={I18n.t("Uma tela • automática"),I18n.t("Duas telas • lado a lado"),I18n.t("Tela inferior ampliada"),I18n.t("Uma tela • batalha: comandos"),I18n.t("Uma tela • batalha: golpes"),I18n.t("Uma tela • outros menus (painel)"),controlsVisible?I18n.t("Ocultar controles 3DS"):I18n.t("Mostrar controles 3DS")};
  AlertDialog dialog=new AlertDialog.Builder(this).setTitle(I18n.t("Modo de tela e interface")).setSingleChoiceItems(choices,screenMode,(d,which)->{if(which<6)setScreenProfile(which);else{controlsVisible=!controlsVisible;if(controlOverlay!=null)controlOverlay.setVisibility(controlsVisible?View.VISIBLE:View.GONE);}d.dismiss();}).setNegativeButton(I18n.t("Cancelar"),null).create();
  dialog.setOnDismissListener(d->{interfaceDialogOpen=false;pausedMenu=false;if(page.equals("game")&&active){nativePause(false);if(performanceHud!=null)performanceHud.start();}});dialog.show();
 }

 void syncScreenMode(int mode){screenMode=mode;}
 void touchFromInterface(int x,int y){touchX=x;touchY=y;sendInput();}
 private void sendInput(){int raw=leftMask|rightMask|gamepadMask;boolean x=(raw&(1<<10))!=0;
  if(x&&(lastInputMask&(1<<10))==0&&screenMode!=1){
   if(nativePopupVisible())xAsBack=true;
   else{screenMode=5;screenPointer=-1;touchX=touchY=-1;nativeOpenPopup();append("X opened corner menu");}
  }
  int output=xAsBack?(raw&~(1<<10))|(x?2:0):raw;
  lastInputMask=raw;nativeInput(output,circleX!=0?circleX:gamepadX,circleY!=0?circleY:gamepadY,touchX,touchY);if(!x)xAsBack=false;
 }

 private int gamepadMask=0,gamepadX=0,gamepadY=0;
 private static int controllerBit(int key){switch(key){
  case KeyEvent.KEYCODE_BUTTON_A:return 1;case KeyEvent.KEYCODE_BUTTON_B:return 2;
  case KeyEvent.KEYCODE_BUTTON_X:return 1<<10;case KeyEvent.KEYCODE_BUTTON_Y:return 1<<11;
  case KeyEvent.KEYCODE_BUTTON_L1:return 1<<9;case KeyEvent.KEYCODE_BUTTON_R1:return 1<<8;
  case KeyEvent.KEYCODE_BUTTON_START:return 8;case KeyEvent.KEYCODE_BUTTON_SELECT:return 4;
  case KeyEvent.KEYCODE_DPAD_RIGHT:return 16;case KeyEvent.KEYCODE_DPAD_LEFT:return 34;
  case KeyEvent.KEYCODE_DPAD_UP:return 64;case KeyEvent.KEYCODE_DPAD_DOWN:return 128;default:return 0;
 }}
 @Override public boolean dispatchKeyEvent(KeyEvent event){
  if(page.equals("game")&&!pausedMenu){int bit=controllerBit(event.getKeyCode());if(bit!=0){if(event.getAction()==KeyEvent.ACTION_DOWN)gamepadMask|=bit;else if(event.getAction()==KeyEvent.ACTION_UP)gamepadMask&=~bit;sendInput();return true;}}
  return super.dispatchKeyEvent(event);
 }
 @Override public boolean onGenericMotionEvent(MotionEvent event){
  if(page.equals("game")&&!pausedMenu&&(event.getSource()&InputDevice.SOURCE_JOYSTICK)==InputDevice.SOURCE_JOYSTICK&&event.getAction()==MotionEvent.ACTION_MOVE){
   float x=event.getAxisValue(MotionEvent.AXIS_X),y=event.getAxisValue(MotionEvent.AXIS_Y);
   gamepadX=Math.abs(x)>.15f?Math.round(x*154):0;gamepadY=Math.abs(y)>.15f?Math.round(-y*154):0;
   gamepadMask&=~240;float hx=event.getAxisValue(MotionEvent.AXIS_HAT_X),hy=event.getAxisValue(MotionEvent.AXIS_HAT_Y);
   if(hx>.5f)gamepadMask|=16;if(hx<-.5f)gamepadMask|=32;if(hy<-.5f)gamepadMask|=64;if(hy>.5f)gamepadMask|=128;
   sendInput();return true;
  }return super.onGenericMotionEvent(event);
 }
 private void releaseInput(){leftMask=rightMask=circleX=circleY=gamepadMask=gamepadX=gamepadY=0;touchX=touchY=-1;screenPointer=-1;sendInput();}
 private void gameMenu(){if(performanceHud!=null)performanceHud.stop();pausedMenu=true;nativePause(true);releaseInput();AlertDialog dialog=new AlertDialog.Builder(this).setTitle("Pokémon X").setItems(new String[]{I18n.t("Continuar"),I18n.t("Modo de tela e interface"),I18n.t("Exportar logs"),I18n.t("Encerrar e voltar ao launcher")},(d,which)->{
  if(which==1){screenOptions();}
  if(which==2)exportLog();
  if(which==3){nativeStop();message=I18n.t("Encerrando e salvando…");showLauncher();}
 }).create();dialog.setOnDismissListener(d->{if(interfaceDialogOpen)return;pausedMenu=false;if(page.equals("game")&&active){nativePause(false);if(performanceHud!=null)performanceHud.start();}});dialog.show();}
 private void showEditor(){if(running.get()||importing)return;stopHud();page="editor";screen=null;surfaceReady=false;nativePause(true);nativeSurface(null);releaseInput();boolean overlay=settings.getBoolean("overlay_controls",true);
  LinearLayout root=column();root.setBackgroundColor(0xff0b121c);FrameLayout area=new FrameLayout(this);TextView info=text(I18n.t("PRÉVIA DA ÁREA DO JOGO\nArraste os botões para reposicionar."),16,0xff54d8dc);info.setGravity(Gravity.CENTER);area.addView(info,new FrameLayout.LayoutParams(-1,-1));
  LinearLayout pads=new LinearLayout(this);ControlPad left=new ControlPad(this,true,true,overlay,(mask,x,y)->{}),right=new ControlPad(this,false,true,overlay,(mask,x,y)->{});
  if(overlay){pads.addView(left,new LinearLayout.LayoutParams(0,-1,1));pads.addView(right,new LinearLayout.LayoutParams(0,-1,1));}else{int rail=Math.min(dp(116),getResources().getDisplayMetrics().widthPixels/5);pads.addView(left,new LinearLayout.LayoutParams(rail,-1));pads.addView(new View(this),new LinearLayout.LayoutParams(0,-1,1));pads.addView(right,new LinearLayout.LayoutParams(rail,-1));}
  area.addView(pads,new FrameLayout.LayoutParams(-1,-1));root.addView(area,new LinearLayout.LayoutParams(-1,0,1));LinearLayout bar=new LinearLayout(this);
  bar.addView(button(I18n.t("Salvar e voltar"),this::showSettings),new LinearLayout.LayoutParams(0,dp(40),1));bar.addView(button(I18n.t("Restaurar"),()->{SharedPreferences prefs=getSharedPreferences("controls",0);SharedPreferences.Editor edit=prefs.edit();String prefix=overlay?"outline_v1_overlay_":"outline_v1_docked_";for(String key:prefs.getAll().keySet())if(key.startsWith(prefix))edit.remove(key);edit.apply();showEditor();}),new LinearLayout.LayoutParams(0,dp(40),1));
  bar.addView(button(I18n.t("Tamanho"),()->{SeekBar size=new SeekBar(this);size.setMax(28);size.setProgress(getSharedPreferences("controls",0).getInt("size",44)-28);size.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){public void onProgressChanged(SeekBar s,int v,boolean user){if(user){getSharedPreferences("controls",0).edit().putInt("size",v+28).apply();left.invalidate();right.invalidate();}}public void onStartTrackingTouch(SeekBar s){}public void onStopTrackingTouch(SeekBar s){}});new AlertDialog.Builder(this).setTitle(I18n.t("Tamanho dos botões")).setView(size).setPositiveButton(I18n.t("Salvar"),null).show();}),new LinearLayout.LayoutParams(0,dp(40),1));root.addView(bar);setContentView(root);
 }
 private void pick(int code){if(importing||running.get())return;Intent i=new Intent(Intent.ACTION_OPEN_DOCUMENT);i.setType("*/*");i.addCategory(Intent.CATEGORY_OPENABLE);startActivityForResult(i,code);}
 private void saveDestination(int request){if(importing||running.get())return;Intent intent=new Intent(Intent.ACTION_CREATE_DOCUMENT);intent.addCategory(Intent.CATEGORY_OPENABLE);intent.setType("application/zip");intent.putExtra(Intent.EXTRA_TITLE,request==13?"PokemonX-save-backup.zip":"PokemonX-save-"+System.currentTimeMillis()+".zip");startActivityForResult(intent,request);}
 private void fileOperation(int request,Uri uri){if(importing||running.get()){toast(I18n.t("Encerre o jogo antes de importar ou exportar arquivos."));return;}importing=true;message=request==10?I18n.t("Instalando pack de texturas…"):request==11?I18n.t("Restaurando save…"):I18n.t("Exportando save…");showSettings();new Thread(()->{File input=null;try{
  if(request==10||request==11){input=File.createTempFile("import-",".part",getCacheDir());try(InputStream in=getContentResolver().openInputStream(uri);FileOutputStream out=new FileOutputStream(input)){if(in==null)throw new IOException(I18n.t("Arquivo indisponível."));ModInstaller.copy(in,out,request==10?256L*1024*1024:64L*1024*1024);out.getFD().sync();}
   if(request==10){int count=ModInstaller.install(input,getFilesDir(),displayName(uri));settings.edit().putBoolean("texture_mod",true).apply();message=I18n.t("Mod instalado: ")+count+I18n.t(" texturas. Ativo na próxima sessão.");}else{SaveManager.importSave(input,getFilesDir());message=I18n.t("Save importado. O save anterior foi preservado em backup.");}
  }else{File snapshot=File.createTempFile("save-export-",".zip",getCacheDir());try{if(request==13){File previous=SaveManager.latestBackup(getFilesDir());if(previous==null)throw new IOException(I18n.t("Nenhum backup disponível."));java.nio.file.Files.copy(previous.toPath(),snapshot.toPath(),java.nio.file.StandardCopyOption.REPLACE_EXISTING);}else SaveManager.exportSave(getFilesDir(),new FileOutputStream(snapshot));try(InputStream in=new FileInputStream(snapshot);OutputStream out=getContentResolver().openOutputStream(uri,"wt")){if(out==null)throw new IOException(I18n.t("Destino indisponível."));ModInstaller.copy(in,out,64L*1024*1024);}message=I18n.t("Save exportado.");}finally{snapshot.delete();}}
  append(message);
 }catch(Exception error){message=I18n.t("Falha: ")+I18n.runtime(error.getMessage()==null?error.getClass().getSimpleName():error.getMessage());append(android.util.Log.getStackTraceString(error));}finally{if(input!=null)input.delete();importing=false;runOnUiThread(this::showSettings);}},"FileOperation").start();}
 private String displayName(Uri uri){try(Cursor c=getContentResolver().query(uri,new String[]{OpenableColumns.DISPLAY_NAME},null,null,null)){if(c!=null&&c.moveToFirst())return c.getString(0);}catch(Exception ignored){}return "Pokémon X";}
 @Override protected void onActivityResult(int request,int result,Intent data){super.onActivityResult(request,result,data);if(request>=10&&request<=13){if(result==RESULT_OK&&data!=null&&data.getData()!=null)fileOperation(request,data.getData());return;}if((request!=7&&request!=8&&request!=9)||result!=RESULT_OK||data==null||data.getData()==null)return;if(running.get()){toast(I18n.t("Encerre o jogo antes de instalar arquivos."));return;}Uri uri=data.getData();String name=displayName(uri);importing=true;message=request==7?I18n.t("Preparando importação do CIA…"):request==9?I18n.t("Validando a atualização…"):I18n.t("Instalando driver…");showSettings();new Thread(()->{
  boolean succeeded=false;
  try{if(request==7){GameImporter.importGame(this,uri,this::runtimeStatus);settings.edit().putString("game_name",name).apply();append("Import verified and completed.");message=I18n.t("Importação concluída.");}else if(request==9){UpdateInstaller.install(this,uri,this::runtimeStatus);append("Verified Pokemon X update1.5 installed; base/save data preserved.");message=I18n.t("Atualização 1.5 instalada.");}else{DriverManager.Driver driver=DriverManager.install(this,uri);settings.edit().putString("driver_path",driver.dir.getAbsolutePath()).apply();append("Driver installed: "+driver.name+" / "+driver.library);message=I18n.t("Driver instalado: ")+driver.name;}succeeded=true;}
  catch(Exception error){append(android.util.Log.getStackTraceString(error));message=I18n.t("Falha: ")+I18n.runtime(error.getMessage()==null?error.getClass().getSimpleName():error.getMessage());}finally{importing=false;final boolean warm=(request==7||request==9)&&succeeded&&settings.getBoolean("warm_after_import",true);runOnUiThread(()->{if(warm)prepareShaders();else showSettings();});}
 },request==7?"GameImport":request==9?"UpdateImport":"DriverImport").start();}
 private void startRuntime(){
  if(!running.compareAndSet(false,true))return;
  final File fresh;
  try{if(sessionLogs==null)throw new IOException(I18n.t("Diretório de logs indisponível"));fresh=sessionLogs.begin(true,versionCode(),System.currentTimeMillis(),logHeader()+"\nGame: "+settings.getString("game_name","Pokémon X")+"\nConfigured renderer="+settings.getInt("renderer",0)+", internal_scale="+settings.getInt("render_scale",1)+", driver="+settings.getString("driver_path","")+"\nJava session start");}
  catch(IOException error){running.set(false);message=I18n.t("Não foi possível iniciar o log: ")+I18n.runtime(error.getMessage()==null?error.getClass().getSimpleName():error.getMessage());showLauncher();return;}
  if(!nativePrepare()){running.set(false);message=I18n.t("A execução anterior está encerrando.");showLauncher();return;}
  sessionLog=fresh;settings.edit().putString("last_session_log",fresh.getName()).commit();
  final boolean warming=preparingShaders;
  new Thread(()->{try{nativeRun(getFilesDir().getAbsolutePath(),getApplicationInfo().nativeLibraryDir,fresh.getAbsolutePath(),warming);}finally{try{SessionLogs.append(fresh,"Java session ended: "+new Date());}catch(IOException ignored){}running.set(false);runOnUiThread(()->{if(warming&&page.equals("shaders")){preparingShaders=false;if(!I18n.failed(message)&&!message.startsWith(I18n.t("Preparação cancelada")))message=I18n.t("Preparação concluída. Novos shaders serão guardados ao jogar.");showSettings();}else if(page.equals("launcher")){message=I18n.t("Execução encerrada. Saves preservados.");showLauncher();}else if(page.equals("settings")){message=I18n.t("Execução encerrada. Saves preservados.");showSettings();}});}},"PokemonRuntime").start();
 }
 public void runtimeStatus(String m){message=I18n.runtime(m);runOnUiThread(()->{if(status!=null&&(page.equals("launcher")||page.equals("settings")||page.equals("shaders")))status.setText(I18n.runtime(m));if(page.equals("game")&&active&&(m.startsWith("Falha")||m.startsWith("Execução interrompida")))new AlertDialog.Builder(this).setTitle(I18n.t("Execução interrompida")).setMessage(I18n.t("Exporte os logs para verificarmos a falha.")).setPositiveButton(I18n.t("Exportar logs"),(d,w)->exportLog()).setNegativeButton(I18n.t("Voltar ao launcher"),(d,w)->{nativeStop();showLauncher();}).show();});}
 private void exportLog(){
  final File source=log();if(source==null||sessionLogs==null){toast(I18n.t("Nenhum log disponível. Inicie o jogo primeiro."));return;}
  try{SessionLogs.append(source,"Export snapshot requested: "+new Date()+"; running="+running.get());File snapshot=sessionLogs.snapshot(source,System.currentTimeMillis());
   Intent send=new Intent(Intent.ACTION_SEND);send.setType("text/plain");Uri uri=new Uri.Builder().scheme("content").authority("org.pokemonx.recomp.logs").appendPath("exports").appendPath(snapshot.getName()).build();send.putExtra(Intent.EXTRA_STREAM,uri);send.putExtra(Intent.EXTRA_SUBJECT,snapshot.getName());send.setClipData(ClipData.newRawUri(snapshot.getName(),uri));send.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);startActivity(Intent.createChooser(send,I18n.t("Compartilhar ")+source.getName()));
  }catch(Exception error){toast(I18n.t("Falha ao exportar o log: ")+I18n.runtime(error.getMessage()==null?error.getClass().getSimpleName():error.getMessage()));}
 }
 @Override protected void onPause(){if(performanceHud!=null)performanceHud.stop();active=false;nativePause(true);releaseInput();super.onPause();}
 @Override protected void onResume(){super.onResume();active=true;if(performanceHud!=null)performanceHud.start();nativePause((!page.equals("game")&&!page.equals("shaders"))||pausedMenu);}
 @Override protected void onDestroy(){stopHud();nativeStop();nativeSurface(null);super.onDestroy();}
 @Override public void onBackPressed(){if(page.equals("shaders")){pendingStart=false;if(running.get()){nativeStop();runtimeStatus(I18n.t("Encerrando a preparação…"));}else{preparingShaders=false;showSettings();}return;}if(page.equals("game")){gameMenu();return;}if(page.equals("editor")){showSettings();return;}if(page.equals("settings")){showLauncher();return;}if(importing){toast(I18n.t("Aguarde o fim da importação."));return;}super.onBackPressed();}
 public void surfaceCreated(SurfaceHolder h){if(screen==null||h!=screen.getHolder())return;surfaceReady=true;nativeSurface(h.getSurface());if(pendingStart){pendingStart=false;startRuntime();}}
 public void surfaceChanged(SurfaceHolder h,int format,int w,int height){if(screen!=null&&h==screen.getHolder())nativeSurface(h.getSurface());}
 public void surfaceDestroyed(SurfaceHolder h){if(screen!=null&&h==screen.getHolder()){surfaceReady=false;nativeSurface(null);}}
}
