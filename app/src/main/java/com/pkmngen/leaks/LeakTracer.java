package com.pkmngen.leaks;
public interface LeakTracer { void expectWeaklyReachable(Object object, String reason); }
