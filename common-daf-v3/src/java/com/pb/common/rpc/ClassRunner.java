package com.pb.common.rpc;

import org.apache.log4j.Logger;

import java.net.URL;
import java.net.URLClassLoader;

/**
 * classloader URL can be passed on command-line "http://localhost:2001/"
 */
public class ClassRunner {

    protected static Logger logger = Logger.getLogger(ClassRunner.class);

    public String classLoaderURL = null;

    public ClassRunner() {
        this(null);
    }

    public ClassRunner(String classLoaderURL) {
        this.classLoaderURL = classLoaderURL;
        logger.info("classLoaderURL = " + classLoaderURL);
    }

    public void runClass(String className) {

        try {

            Class cls = null;
            if (classLoaderURL != null) {
                cls = loadFromURL(className);
            } else {
                cls = Class.forName(className);
            }

            Object obj = cls.newInstance();
            ((Runnable) obj).run();
        }
        catch (Exception e) {
            logger.error(e);
        }
    }

    private Class loadFromURL(String className) throws Exception {
        URL url = new URL(classLoaderURL);
        URL[] urls = new URL[]{url};

        URLClassLoader cl = new URLClassLoader(urls);
        Class cls = cl.loadClass(className);

        logger.debug(cls.toString());

        return cls;
    }

    public static void main(String[] args) {

        String classLoaderURL = null;

        if (args.length > 1) {
            classLoaderURL = args[0];
        }

        if (classLoaderURL != null) {
            new ClassRunner(classLoaderURL);
        } else {
            new ClassRunner();
        }
    }
}
