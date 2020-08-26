package com.pb.common.rpc.tests;

import javax.swing.*;
import java.io.File;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.io.FileNotFoundException;

public class TestClass {

    public TestClass()
    {
        System.out.println("hello from stdout");
        System.err.println("hello from stderr");
        JOptionPane.showMessageDialog(null, "hello world");
    }

    public static void main(String[] args)
    {
        new TestClass();
    }
}
