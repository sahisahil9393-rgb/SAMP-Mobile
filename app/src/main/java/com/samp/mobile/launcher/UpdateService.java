package com.samp.mobile.launcher;

import android.app.Service;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.util.Log;

import com.android.volley.DefaultRetryPolicy;
import com.android.volley.Request;
import com.android.volley.Response;
import com.android.volley.VolleyError;
import com.android.volley.toolbox.StringRequest;
import com.android.volley.toolbox.Volley;
import com.downloader.Error;
import com.downloader.OnDownloadListener;
import com.downloader.OnProgressListener;
import com.downloader.PRDownloader;
import com.downloader.PRDownloaderConfig;
import com.downloader.Progress;
import com.joom.paranoid.Obfuscate;
import com.samp.mobile.launcher.data.FilesData;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.util.ArrayList;
@Obfuscate
public class UpdateService extends Service {

    Messenger mMessenger;
    Messenger mActivityMessenger;
    IncomingHandler mInHandler;

    public UpdateActivity.GameStatus mGameStatus = UpdateActivity.GameStatus.Unknown;
    public UpdateActivity.UpdateStatus mUpdateStatus = UpdateActivity.UpdateStatus.Undefined;

    public boolean mDownloadingStatus = false;

    public long mUpdateGameDataSize = 0;
    public long mUpdateGameDataSizeUpdated = 0;
    public String mUpdateGameURL = "";
    public int mUpdateVersion;

    public ArrayList<String> mUpdateFiles;
    public ArrayList<String> mUpdateFilesName;
    public ArrayList<Long> mUpdateFilesSize;
    public ArrayList<String> mUpdateFileUrls;

    public int mGpuType = 0;

    private static final String CLIENT_CONFIG_URL =
            "https://raw.githubusercontent.com/sahisahil9393-rgb/my-skins/main/client_config.json";
    private static final int UPDATE_REQUEST_TIMEOUT_MS = 30000;

    @Override
    public void onCreate() {
        super.onCreate();
        HandlerThread handlerThread = new HandlerThread("ServiceStartArguments", 10);
        handlerThread.start();
        PRDownloader.initialize(getApplicationContext(), PRDownloaderConfig.newBuilder().setDatabaseEnabled(true).setReadTimeout(30000).setConnectTimeout(30000).build());
        mInHandler = new IncomingHandler(handlerThread.getLooper());
        mMessenger = new Messenger(mInHandler);
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mMessenger.getBinder();
    }

    class IncomingHandler extends Handler {
        public IncomingHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            mActivityMessenger = msg.replyTo;
            Message obtain = null;
            Messenger messenger = null;
            if (msg.what == 0) {
                mGpuType = msg.getData().getInt("gputype");
                if(mGpuType == 0)
                {
                    Log.e("x1y2z", "GPU not found");
                    return;
                }
                startUpdating();
            } else if (msg.what == 1) {
                startGameUpdateChecking();
            } else if(msg.what == 2) {
                updateGame();
            } else if (msg.what == 4) {
                obtain = Message.obtain(mInHandler, 4);
                obtain.getData().putString("status", mUpdateStatus.name());
                UpdateService updateService = UpdateService.this;
                obtain.replyTo = updateService.mMessenger;
                messenger = updateService.mActivityMessenger;
                if (messenger != null) {
                    try {
                        messenger.send(obtain);
                    } catch (RemoteException e5) {
                        Log.e("UpdateService", "Unable to send update status", e5);
                    }
                }
            } else if (msg.what == 5) {
                obtain = Message.obtain(mInHandler, 5);
                obtain.getData().putString("status", mGameStatus.name());
                obtain.replyTo = mMessenger;
                if (mActivityMessenger != null) {
                    try {
                        mActivityMessenger.send(obtain);
                    } catch (RemoteException e5) {
                        Log.e("UpdateService", "Unable to send game status", e5);
                    }
                }

            } else if (msg.what == 7) {
                mGpuType = msg.getData().getInt("gputype");
                if(mGpuType == 0)
                {
                    Log.e("x1y2z", "GPU not found");
                    return;
                }
                startUpdating();
            }

        }
    }

    void startUpdating()
    {
        setUpdateStatus(UpdateActivity.UpdateStatus.CheckUpdate);
        mUpdateFiles = new ArrayList<>();
        mUpdateFilesName = new ArrayList<>();
        mUpdateFilesSize = new ArrayList<>();
        mUpdateFileUrls = new ArrayList<>();
        mUpdateGameDataSize = 0;
        mUpdateGameDataSizeUpdated = 0;

        StringRequest request = new StringRequest(Request.Method.GET, CLIENT_CONFIG_URL, new Response.Listener<String>() {
            @Override
            public void onResponse(String response) {
                try {
                    JSONObject root = new JSONObject(response);
                    /*
                     * Older configurations wrapped these values in client_config.
                     * The current my-skins/client_config.json stores them at the root.
                     * Supporting both formats prevents the launcher from getting stuck
                     * when the configuration repository changes shape.
                     */
                    JSONObject jSONObject = root.optJSONObject("client_config");
                    if (jSONObject == null) {
                        jSONObject = root;
                    }
                    mUpdateVersion = jSONObject.getInt("version_code");
                    mUpdateGameURL = jSONObject.optString("url_launcher", "");

                    String filesUrl = jSONObject.getString("url_cache_files");
                    getFilesInfo(filesUrl);

                } catch (JSONException e) {
                    failUpdate("Invalid update configuration", e);
                }
            }
        }, new Response.ErrorListener() {
            @Override
            public void onErrorResponse(VolleyError error) {
                failUpdate("Unable to load update configuration", error);
            }
        });
        request.setRetryPolicy(new DefaultRetryPolicy(
                UPDATE_REQUEST_TIMEOUT_MS,
                1,
                DefaultRetryPolicy.DEFAULT_BACKOFF_MULT
        ));
        Volley.newRequestQueue(getApplicationContext()).add(request);
    }

    private void getFilesInfo(String filesUrl) {
        StringRequest request = new StringRequest(Request.Method.GET, filesUrl, new Response.Listener<String>() {
            @Override
            public void onResponse(String response) {
                try {
                    parseFilesInfo(response);
                    completeUpdateCheck();
                } catch (JSONException e) {
                    failUpdate("Invalid update file list", e);
                }
            }
        }, new Response.ErrorListener() {
            @Override
            public void onErrorResponse(VolleyError error) {
                failUpdate("Unable to load update file list", error);
            }
        });
        request.setRetryPolicy(new DefaultRetryPolicy(
                UPDATE_REQUEST_TIMEOUT_MS,
                1,
                DefaultRetryPolicy.DEFAULT_BACKOFF_MULT
        ));
        Volley.newRequestQueue(getApplicationContext()).add(request);
    }

    private void parseFilesInfo(String response) throws JSONException {
        JSONObject jsonObject = new JSONObject(response);
        JSONArray jsonArray = jsonObject.getJSONArray("files");
        Log.d("x1y2z", "Length: " + jsonArray.length());
        File externalFilesDir = getExternalFilesDir(null);
        if (externalFilesDir == null) {
            throw new JSONException("External files directory is unavailable");
        }

        for(int i = 0; i<jsonArray.length(); i++) {
            JSONObject fileObject = jsonArray.getJSONObject(i);
            String name = fileObject.optString("name", "");
            long size = fileObject.optLong("size", -1);
            String path = fileObject.optString("path", "");
            String url = fileObject.optString("url", "");
            if (name.length() == 0 || path.length() == 0 || url.length() == 0 || size < 0) {
                throw new JSONException("Invalid file entry at index " + i);
            }

            FilesData fileData = new FilesData(name, size, path, url);
            if (!fileData.getName().equals("samp_log.txt") && !fileData.getName().equals("svlog.txt") && !fileData.getName().equals("gtasatelem.set")) {
                if (!fileData.getName().equals("GTASAMP10.b") && !fileData.getName().equals(".htaccess")) {
                    if (!fileData.getName().equals("gta_sa.set")) {
                        if (!fileData.getName().equals("settings.ini")) {
                            File file = new File(externalFilesDir, fileData.getPath());
                            if (!file.exists() || file.length() != fileData.getSize()) {
                                if(!fileData.getPath().contains("player") && !fileData.getPath().contains("playerhi") && !fileData.getPath().contains("menu") && !fileData.getPath().contains("samp")) {
                                    if ((fileData.getPath().contains(".dxt.") && mGpuType != 1))
                                        continue;
                                    else if ((fileData.getPath().contains(".etc.") && mGpuType != 2))
                                        continue;
                                    else if ((fileData.getPath().contains(".pvr.") && mGpuType != 3))
                                        continue;
                                }

                                mUpdateFiles.add(fileData.getPath());
                                Log.d("x1y2z", "File name: " + fileData.getName());
                                mUpdateFilesName.add(fileData.getName());
                                Log.d("x1y2z", "File path: " + fileData.getPath());
                                mUpdateFilesSize.add(fileData.getSize());
                                mUpdateFileUrls.add(fileData.getUrl());
                                mUpdateGameDataSize=mUpdateGameDataSize+fileData.getSize();
                                Log.d("x1y2z", "File size: " + fileData.getSize());
                            }
                            Log.d("x1y2z", "Data size: " + mUpdateGameDataSize);
                        }
                    }
                }
            }
        }
    }

    private void completeUpdateCheck() {
        boolean gameUpdateExists = isGameUpdateExists();
        if (!gameUpdateExists) {
            if (mUpdateFiles.isEmpty()) {
                mGameStatus = UpdateActivity.GameStatus.Updated;
            } else {
                mGameStatus = UpdateActivity.GameStatus.UpdateRequired;
            }
        } else {
            if (mUpdateGameURL == null || mUpdateGameURL.trim().length() == 0) {
                failUpdate("Update configuration has no APK download URL", null);
                return;
            }
            mGameStatus = UpdateActivity.GameStatus.GameUpdateRequired;
        }
        setUpdateStatus(UpdateActivity.UpdateStatus.Undefined);
    }

    private void failUpdate(String message, Throwable error) {
        Log.e("UpdateService", message, error);
        mGameStatus = UpdateActivity.GameStatus.Unknown;
        setUpdateStatus(UpdateActivity.UpdateStatus.Undefined);
    }

    public void updateGame() {
        if (isGameUpdateExists()) {
            Log.d("UpdateService", "updateGame exists");
            //setUpdateStatus(UpdateActivity.UpdateStatus.DownloadGame);
            downloadGame();
            return;
        }
        Log.d("UpdateService", "updateGame done");
        //setUpdateStatus(UpdateActivity.UpdateStatus.Undefined);
        File file = new File(getExternalFilesDir(null) + "/download/update.apk");
        Message obtain = Message.obtain(mInHandler, 2);
        obtain.getData().putBoolean("status", true);
        obtain.getData().putString("apkPath", file.getAbsolutePath());
        obtain.replyTo = mMessenger;
        if (mActivityMessenger != null) {
            try {
                mActivityMessenger.send(obtain);
            } catch (RemoteException e5) {
                e5.printStackTrace();
            }
        }
        //setUpdateStatus(UpdateActivity.UpdateStatus.Undefined);
    }


    public void startGameUpdateChecking()
    {
        if (!mUpdateFiles.isEmpty()) {
            setUpdateStatus(UpdateActivity.UpdateStatus.DownloadGameData);
            startDataUpdating();
            return;
        }

        Log.d("UpdateService", "updateGameData()");
        Message obtain = Message.obtain(this.mInHandler, 1);
        obtain.getData().putBoolean("status", true);
        obtain.replyTo = mMessenger;
        if (mActivityMessenger != null) {
            try {
                mActivityMessenger.send(obtain);
            } catch (RemoteException e5) {
                e5.printStackTrace();
            }
        }

    }

    public void setUpdateStatus(UpdateActivity.UpdateStatus updateStatus) {
        if (updateStatus.name().length() != 0 && mUpdateStatus != updateStatus) {
            mUpdateStatus = updateStatus;
            Message obtain = Message.obtain(mInHandler, 4);
            obtain.getData().putString("status", mUpdateStatus.name());
            obtain.replyTo = mMessenger;
            Messenger messenger = mActivityMessenger;
            if (messenger != null) {
                try {
                    messenger.send(obtain);
                } catch (RemoteException e5) {
                    e5.printStackTrace();
                }
            }
        }
    }


    public void startDataUpdating()
    {
        ArrayList arrayList = new ArrayList(mUpdateFiles);
        ArrayList arrayList1 = new ArrayList(mUpdateFilesName);
        ArrayList arrayList2 = new ArrayList(mUpdateFilesSize);
        ArrayList arrayList3 = new ArrayList(mUpdateFileUrls);
        mUpdateFiles.clear();
        mUpdateFilesName.clear();
        mUpdateFilesSize.clear();
        mUpdateFileUrls.clear();
        Ref.IntRef intRef = new Ref.IntRef();
        intRef.element = 0;
        Ref.LongRef longRef1 = new Ref.LongRef();
        longRef1.element = 0;
        while(intRef.element<arrayList.size()) {
            mDownloadingStatus = true;

            String string = getExternalFilesDir(null) + "/" + arrayList.get(intRef.element);

            //Log.d("x1y2z", "Update file path: " + string.replace((CharSequence) arrayList1.get(intRef.element), "") + ", Name:" + arrayList1.get(intRef.element));

            File file = new File(getExternalFilesDir(null), string);
            file.getParentFile().mkdirs();
            if (file.exists()) {
                file.delete();
            }

            Ref.LongRef longRef = new Ref.LongRef();
            longRef.element = System.currentTimeMillis();

            Log.d("x1y2z", "startDataUpdating " + mUpdateGameDataSize + " " + mUpdateGameDataSizeUpdated);

            mDownloadingStatus = true;
            PRDownloader.download(String.valueOf(arrayList3.get(intRef.element)), string.replace(arrayList1.get(intRef.element).toString(), ""), String.valueOf(arrayList1.get(intRef.element))).build().setOnStartOrResumeListener(null).setOnPauseListener(null).setOnCancelListener(null).setOnProgressListener(new OnProgressListener() {
                @Override
                public void onProgress(Progress progress) {
                    mDownloadingStatus = true;
                    if(System.currentTimeMillis() - longRef.element > 100) {
                        longRef.element = System.currentTimeMillis();
                        Message obtain = Message.obtain(mInHandler, 4);
                        obtain.getData().putString("status", UpdateActivity.UpdateStatus.DownloadGameData.name());
                        obtain.getData().putBoolean("withProgress", true);
                        obtain.getData().putLong("current", longRef1.element+progress.currentBytes);
                        obtain.getData().putLong("total", mUpdateGameDataSize/2);
                        obtain.getData().putString("filename", (String)arrayList1.get(intRef.element));
                        obtain.getData().putLong("totalfiles", arrayList.size());
                        obtain.getData().putLong("currentfile", intRef.element);
                        if (mActivityMessenger != null) {
                            try {
                                mActivityMessenger.send(obtain);
                            } catch (RemoteException e) {
                                e.printStackTrace();
                            }
                        }
                    }

                }
            }).start(new OnDownloadListener() {
                @Override
                public void onDownloadComplete() {
                    mDownloadingStatus = false;
                    longRef1.element+=(long)arrayList2.get(intRef.element);
                    Log.d("x1y2z", "completed");
                }

                @Override
                public void onError(Error error) {
                    mDownloadingStatus = false;
                    mUpdateFiles.add(String.valueOf(arrayList.get(intRef.element)));
                    Log.d("x1y2z", "error downloadgamedata");
                }
            });

            do {
                try {
                    Thread.sleep(30);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            } while (mDownloadingStatus);


            intRef.element++;

        }

        mDownloadingStatus = false;

        Log.d("UpdateService", "updateGameData()");
        Message obtain = Message.obtain(this.mInHandler, 1);
        obtain.getData().putBoolean("status", true);
        obtain.replyTo = mMessenger;
        if (mActivityMessenger != null) {
            try {
                mActivityMessenger.send(obtain);
            } catch (RemoteException e5) {
                e5.printStackTrace();
            }
        }
    }

    public void downloadGame()
    {
        Log.d("UpdateService", "downloadGame");
        if (mUpdateGameURL == null || mUpdateGameURL.trim().length() == 0) {
            failUpdate("Cannot download game update: APK URL is empty", null);
            return;
        }

        File externalFilesDir = getExternalFilesDir(null);
        if (externalFilesDir == null) {
            failUpdate("Cannot download game update: external files directory is unavailable", null);
            return;
        }

        mDownloadingStatus = true;

        File file = new File(externalFilesDir, "download/update.apk");
        if (file.exists()) {
            file.delete();
        }

        Ref.LongRef longRef = new Ref.LongRef();
        longRef.element = System.currentTimeMillis();

        mDownloadingStatus = true;
        PRDownloader.download(mUpdateGameURL, new File(externalFilesDir, "download").getAbsolutePath(), "update.apk").build().setOnStartOrResumeListener(null).setOnPauseListener(null).setOnCancelListener(null).setOnProgressListener(new OnProgressListener() {
            @Override
            public void onProgress(Progress progress) {
                mDownloadingStatus = true;
                if(System.currentTimeMillis() - longRef.element > 100) {
                    longRef.element = System.currentTimeMillis();
                    Message obtain = Message.obtain(mInHandler, 4);
                    obtain.getData().putString("status", UpdateActivity.UpdateStatus.DownloadGame.name());
                    obtain.getData().putBoolean("withProgress", true);
                    obtain.getData().putLong("current", progress.currentBytes);
                    obtain.getData().putLong("total", progress.totalBytes);
                    obtain.getData().putString("filename", "update.apk");
                    obtain.getData().putLong("totalfiles", 1);
                    obtain.getData().putLong("currentfile", 1);
                    if (mActivityMessenger != null) {
                        try {
                            mActivityMessenger.send(obtain);
                        } catch (RemoteException e) {
                            e.printStackTrace();
                        }
                    }
                }

            }
        }).start(new OnDownloadListener() {
            @Override
            public void onDownloadComplete() {
                Message obtain = Message.obtain(UpdateService.this.mInHandler, 2);
                obtain.getData().putBoolean("status", true);
                obtain.getData().putString("apkPath", file.getAbsolutePath());
                obtain.replyTo = mMessenger;
                if (mActivityMessenger != null) {
                    try {
                        mActivityMessenger.send(obtain);
                    } catch (RemoteException e5) {
                        e5.printStackTrace();
                    }
                }
                setUpdateStatus(UpdateActivity.UpdateStatus.Undefined);
                mDownloadingStatus = false;
                Log.d("x1y2z", "completed");
            }

            @Override
            public void onError(Error error) {
                mDownloadingStatus = false;
                failUpdate("Unable to download game update", error);
            }
        });

        do {
            try {
                Thread.sleep(30);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        } while (mDownloadingStatus);

        mDownloadingStatus = false;
    }

    public boolean isGameUpdateExists() {
        PackageInfo packageInfo = null;
        try {
            packageInfo = getPackageManager().getPackageInfo("com.samp.mobile", PackageManager.GET_ACTIVITIES);
        } catch (PackageManager.NameNotFoundException e) {
            e.printStackTrace();
        }
        Log.d("x1y2z", "isGameUpdateExists -> currentVersion " + packageInfo.versionCode + " | mUpdateVersion " + this.mUpdateVersion);
        return packageInfo.versionCode == this.mUpdateVersion ? false:true;
    }

    private void sendLoadingScreen(boolean unpacking, String filename, long current, long total) {
        new Thread(new Runnable() {
            public void run() {
                Message obtain = Message.obtain(UpdateService.this.mInHandler, 4);
                obtain.getData().putString("status", UpdateActivity.UpdateStatus.CheckUpdate.name());
                obtain.getData().putBoolean("withProgress", true);
                obtain.getData().putString("filename", filename);
                obtain.getData().putBoolean("unpacking", unpacking);
                obtain.getData().putLong("current", current);
                obtain.getData().putLong("total", total);
                obtain.replyTo = mMessenger;
                if (mActivityMessenger != null) {
                    try {
                        mActivityMessenger.send(obtain);
                    } catch (RemoteException e5) {
                        e5.printStackTrace();
                    }
                }
            }
        }).start();
    }


}
