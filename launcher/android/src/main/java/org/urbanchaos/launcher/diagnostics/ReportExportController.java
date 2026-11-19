package org.urbanchaos.launcher.diagnostics;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.Button;
import android.widget.Toast;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/** Attach to the real game Activity; this class never starts or simulates a game. */
public final class ReportExportController implements AutoCloseable {
    public static final int REQUEST=0x5543;
    private final Activity activity;
    private final AndroidReports reports;
    private final ExecutorService worker=Executors.newSingleThreadExecutor();
    private MenuItem item;
    private Button button;
    private boolean busy,closed;

    public ReportExportController(Activity activity,AndroidReports applicationReports){
        this.activity=activity;reports=applicationReports;
    }
    public void addTo(Menu menu){
        item=menu.add(Menu.NONE,0x55430001,Menu.NONE,"Exportar relatório");
        item.setShowAsAction(MenuItem.SHOW_AS_ACTION_NEVER);
        item.setOnMenuItemClickListener(unused->{selectDestination();return true;});
        enabled();
    }
    public Button createButton(){
        button=new Button(activity);button.setText("Exportar relatório");
        button.setOnClickListener(unused->selectDestination());enabled();return button;
    }
    private void enabled(){
        if(item!=null)item.setEnabled(!busy&&!closed);
        if(button!=null)button.setEnabled(!busy&&!closed);
    }
    @SuppressWarnings("deprecation")
    public void selectDestination(){
        if(closed||busy)return;
        busy=true;enabled();
        try{activity.startActivityForResult(reports.createExportDocument(),REQUEST);}
        catch(ActivityNotFoundException|SecurityException error){
            reports.session().event(ReportSession.Event.EXPORT_PICKER_UNAVAILABLE,java.util.Collections.emptyMap());
            busy=false;enabled();show("Seletor de arquivos indisponível (UC-EXPORT-003)");
        }
    }
    /** Forward from Activity.onActivityResult; true means the result was handled here. */
    public boolean handleResult(int request,int result,Intent data){
        if(request!=REQUEST)return false;
        if(closed)return true;
        if(result!=Activity.RESULT_OK||data==null||data.getData()==null){
            reports.session().event(ReportSession.Event.EXPORT_CANCELLED,java.util.Collections.emptyMap());
            busy=false;enabled();show("Exportação cancelada");return true;
        }
        busy=true;enabled();
        reports.exportTo(data.getData(),worker,activity::runOnUiThread,(success,code)->{
            busy=false;enabled();
            show(success?"Relatório salvo":"Falha ao exportar ("+code+")");
        });
        return true;
    }
    private void show(String message){
        if(!closed&&!activity.isFinishing()&&!activity.isDestroyed())
            Toast.makeText(activity,message,Toast.LENGTH_LONG).show();
    }
    // Activity recreation must not close the application-scoped diagnostic session.
    @Override public void close(){closed=true;enabled();worker.shutdown();}
}
