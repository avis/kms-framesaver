/**
 * This file is generated with Kurento-maven-plugin.
 * Please don't edit.
 */
package org.kurento.module.framesavervideofilter;

import org.kurento.client.*;

/**
 *
 * FrameSaverVideoFilter interface --- saves frames as PNG files.
 *
 **/
@org.kurento.client.internal.RemoteClass
public interface FrameSaverVideoFilter extends Filter {



/**
 *
 * changes pipeline state to PLAYING
 * @return FALSE when Failed. *
 **/
  boolean startPipelinePlaying();

/**
 *
 * Asynchronous version of startPipelinePlaying:
 * {@link Continuation#onSuccess} is called when the action is
 * done. If an error occurs, {@link Continuation#onError} is called.
 * @see FrameSaverVideoFilter#startPipelinePlaying
 *
 **/
    void startPipelinePlaying(Continuation<Boolean> cont);

/**
 *
 * changes pipeline state to PLAYING
 * @return FALSE when Failed. *
 **/
    TFuture<Boolean> startPipelinePlaying(Transaction tx);


/**
 *
 * changes pipeline state from PLAYING to READY
 * @return FALSE when Failed. *
 **/
  boolean stopPipelinePlaying();

/**
 *
 * Asynchronous version of stopPipelinePlaying:
 * {@link Continuation#onSuccess} is called when the action is
 * done. If an error occurs, {@link Continuation#onError} is called.
 * @see FrameSaverVideoFilter#stopPipelinePlaying
 *
 **/
    void stopPipelinePlaying(Continuation<Boolean> cont);

/**
 *
 * changes pipeline state from PLAYING to READY
 * @return FALSE when Failed. *
 **/
    TFuture<Boolean> stopPipelinePlaying(Transaction tx);


/**
 *
 * gets a string of names of all elements separated by tabs.
 * @return names of all elements separated by tabs --- pipeline name is the first element *
 **/
  String getElementsNamesList();

/**
 *
 * Asynchronous version of getElementsNamesList:
 * {@link Continuation#onSuccess} is called when the action is
 * done. If an error occurs, {@link Continuation#onError} is called.
 * @see FrameSaverVideoFilter#getElementsNamesList
 *
 **/
    void getElementsNamesList(Continuation<String> cont);

/**
 *
 * gets a string of names of all elements separated by tabs.
 * @return names of all elements separated by tabs --- pipeline name is the first element *
 **/
    TFuture<String> getElementsNamesList(Transaction tx);


/**
 *
 * gets string of last error --- empty string when no error
 * @return string of last error --- empty string when no error *
 **/
  String getLastError();

/**
 *
 * Asynchronous version of getLastError:
 * {@link Continuation#onSuccess} is called when the action is
 * done. If an error occurs, {@link Continuation#onError} is called.
 * @see FrameSaverVideoFilter#getLastError
 *
 **/
    void getLastError(Continuation<String> cont);

/**
 *
 * gets string of last error --- empty string when no error
 * @return string of last error --- empty string when no error *
 **/
    TFuture<String> getLastError(Transaction tx);


/**
 *
 * gets a string of all parameters separated by tabs.
 * @return all parameters separated by tabs --- each one is: name=value *
 **/
  String getParamsList();

/**
 *
 * Asynchronous version of getParamsList:
 * {@link Continuation#onSuccess} is called when the action is
 * done. If an error occurs, {@link Continuation#onError} is called.
 * @see FrameSaverVideoFilter#getParamsList
 *
 **/
    void getParamsList(Continuation<String> cont);

/**
 *
 * gets a string of all parameters separated by tabs.
 * @return all parameters separated by tabs --- each one is: name=value *
 **/
    TFuture<String> getParamsList(Transaction tx);


/**
 *
 * gets the current string value of one parameter.
 *
 * @param aParamName
 *       string with name of parameter.
 * @return current value of named parameter --- empty if invalid name *
 **/
  String getParam(@org.kurento.client.internal.server.Param("aParamName") String aParamName);

/**
 *
 * Asynchronous version of getParam:
 * {@link Continuation#onSuccess} is called when the action is
 * done. If an error occurs, {@link Continuation#onError} is called.
 * @see FrameSaverVideoFilter#getParam
 *
 * @param aParamName
 *       string with name of parameter.
 *
 **/
    void getParam(@org.kurento.client.internal.server.Param("aParamName") String aParamName, Continuation<String> cont);

/**
 *
 * gets the current string value of one parameter.
 *
 * @param aParamName
 *       string with name of parameter.
 * @return current value of named parameter --- empty if invalid name *
 **/
    TFuture<String> getParam(Transaction tx, @org.kurento.client.internal.server.Param("aParamName") String aParamName);


/**
 *
 * sets the current string value of one parameter.
 *
 * @param aParamName
 *       string with name of parameter.
 * @param aNewParamValue
 *       string has the desired value of the parameter.
 * @return FALSE when Failed. *
 **/
  boolean setParam(@org.kurento.client.internal.server.Param("aParamName") String aParamName, @org.kurento.client.internal.server.Param("aNewParamValue") String aNewParamValue);

/**
 *
 * Asynchronous version of setParam:
 * {@link Continuation#onSuccess} is called when the action is
 * done. If an error occurs, {@link Continuation#onError} is called.
 * @see FrameSaverVideoFilter#setParam
 *
 * @param aParamName
 *       string with name of parameter.
 * @param aNewParamValue
 *       string has the desired value of the parameter.
 *
 **/
    void setParam(@org.kurento.client.internal.server.Param("aParamName") String aParamName, @org.kurento.client.internal.server.Param("aNewParamValue") String aNewParamValue, Continuation<Boolean> cont);

/**
 *
 * sets the current string value of one parameter.
 *
 * @param aParamName
 *       string with name of parameter.
 * @param aNewParamValue
 *       string has the desired value of the parameter.
 * @return FALSE when Failed. *
 **/
    TFuture<Boolean> setParam(Transaction tx, @org.kurento.client.internal.server.Param("aParamName") String aParamName, @org.kurento.client.internal.server.Param("aNewParamValue") String aNewParamValue);

    



    public class Builder extends AbstractBuilder<FrameSaverVideoFilter> {

/**
 *
 * Creates a Builder for FrameSaverVideoFilter
 *
 **/
    public Builder(org.kurento.client.MediaPipeline mediaPipeline){

      super(FrameSaverVideoFilter.class,mediaPipeline);

      props.add("mediaPipeline",mediaPipeline);
    }

	public Builder withProperties(Properties properties) {
    	return (Builder)super.withProperties(properties);
  	}

	public Builder with(String name, Object value) {
		return (Builder)super.with(name, value);
	}
	
    }


}