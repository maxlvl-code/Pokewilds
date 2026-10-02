package gme;
public class VGMPlayer implements Runnable {
    private boolean playing;
    private double volume = 1.0;
    private int currentTrack;
    public VGMPlayer(int sampleRate) {}
    public void loadFile(String url, String path) throws Exception {}
    public void run() {}
    public void stop() throws Exception { playing = false; }
    public void play() throws Exception { playing = true; }
    public boolean isPlaying() { return playing; }
    public void pause() throws Exception { playing = false; }
    public double getVolume() { return volume; }
    public void setVolume(double v) { volume = v; }
    public int getCurrentTime() { return 0; }
    public int getCurrentTrack() { return currentTrack; }
    public void startTrack(int track, int seconds) throws Exception { currentTrack = track; playing = true; }
    public int getTrackCount() { return 0; }
}
