package ag.boersego.bgjs;

/**
 * Created by kread on 21/09/15.
 */
public interface IV8GLViewOnRender {
    void renderStarted(V8TextureView instance);
    void renderThreadClosed(V8TextureView instance);
}
