import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.util.jar.*;
import jdk.internal.org.objectweb.asm.*;

public final class AndroidJarPatcher {
    private static final Set<String> EXACT_EXCLUDES = Set.of(
        "com/pkmngen/updater/VersionControl.class",
        "com/pkmngen/leaks/LeakTracer.class"
    );

    private static final String[] PREFIX_EXCLUDES = {
        "com/pkmngen/game/desktop/",
        "com/pkmngen/leaks/",
        "com/pkmngen/updater/",
        "com/badlogic/",
        "org/lwjgl/",
        "org/lwjgl3/",
        "com/studiohartman/",
        "oshi/",
        "com/sun/jna/",
        "net/harawata/",
        "leakcanary/",
        "shark/",
        "kotlin/",
        "org/intellij/",
        "org/jetbrains/",
        "META-INF/versions/",
        "META-INF/native/",
        "com/pkmngen/game/util/Dirs",
        "okio/",
        "gme/"
    };

    private static boolean excluded(String name) {
        if (EXACT_EXCLUDES.contains(name)) return true;
        for (String prefix : PREFIX_EXCLUDES) if (name.startsWith(prefix)) return true;
        String lower = name.toLowerCase(Locale.ROOT);
        return lower.endsWith(".dll") || lower.endsWith(".so") ||
               lower.endsWith(".dylib") || lower.endsWith(".jnilib") ||
               lower.endsWith(".exe");
    }

    private static byte[] patchClass(String name, byte[] input) {
        if (!name.equals("com/pkmngen/game/Game.class") &&
            !name.equals("com/pkmngen/game/DrawSetupMenu.class")) {
            return input;
        }

        ClassReader cr = new ClassReader(input);
        ClassWriter cw = new ClassWriter(0);
        ClassVisitor cv = new ClassVisitor(Opcodes.ASM7, cw) {
            @Override
            public MethodVisitor visitMethod(int access, String methodName, String desc,
                                             String sig, String[] ex) {
                MethodVisitor base = super.visitMethod(access, methodName, desc, sig, ex);
                return new MethodVisitor(Opcodes.ASM7, base) {
                    int frameState = 0;

                    @Override
                    public void visitTypeInsn(int opcode, String type) {
                        if (name.equals("com/pkmngen/game/DrawSetupMenu.class") &&
                            opcode == Opcodes.NEW && type.equals("javax/swing/JFrame")) {
                            super.visitInsn(Opcodes.ACONST_NULL);
                            frameState = 1;
                            return;
                        }
                        super.visitTypeInsn(opcode, type);
                    }

                    @Override
                    public void visitInsn(int opcode) {
                        if (frameState == 1 && opcode == Opcodes.DUP) {
                            frameState = 2;
                            return;
                        }
                        super.visitInsn(opcode);
                    }

                    @Override
                    public void visitLdcInsn(Object value) {
                        if (frameState == 2 && "Error".equals(value)) {
                            frameState = 3;
                            return;
                        }
                        super.visitLdcInsn(value);
                    }

                    @Override
                    public void visitFrame(int type, int numLocal, Object[] local,
                                           int numStack, Object[] stack) {
                        Object[] l2 = local == null ? null : local.clone();
                        Object[] s2 = stack == null ? null : stack.clone();
                        if (l2 != null) for (int i = 0; i < l2.length; i++)
                            if ("javax/swing/JFrame".equals(l2[i])) l2[i] = "java/lang/Object";
                        if (s2 != null) for (int i = 0; i < s2.length; i++)
                            if ("javax/swing/JFrame".equals(s2[i])) s2[i] = "java/lang/Object";
                        super.visitFrame(type, numLocal, l2, numStack, s2);
                    }

                    @Override
                    public void visitMethodInsn(int opcode, String owner, String mname,
                                                String mdesc, boolean itf) {
                        if (name.equals("com/pkmngen/game/DrawSetupMenu.class") &&
                            frameState == 3 &&
                            opcode == Opcodes.INVOKESPECIAL &&
                            owner.equals("javax/swing/JFrame") &&
                            mname.equals("<init>") &&
                            mdesc.equals("(Ljava/lang/String;)V")) {
                            frameState = 0;
                            return;
                        }

                        if (owner.equals("javax/swing/JOptionPane") &&
                            mname.equals("showMessageDialog") &&
                            mdesc.equals("(Ljava/awt/Component;Ljava/lang/Object;)V")) {
                            super.visitInsn(Opcodes.POP2);
                            return;
                        }

                        if (owner.equals("javax/swing/JOptionPane") &&
                            mname.equals("showConfirmDialog") &&
                            mdesc.equals("(Ljava/awt/Component;Ljava/lang/Object;Ljava/lang/String;I)I")) {
                            super.visitInsn(Opcodes.POP);
                            super.visitInsn(Opcodes.POP);
                            super.visitInsn(Opcodes.POP);
                            super.visitInsn(Opcodes.POP);
                            super.visitInsn(Opcodes.ICONST_0);
                            return;
                        }
                        super.visitMethodInsn(opcode, owner, mname, mdesc, itf);
                    }
                };
            }
        };
        cr.accept(cv, 0);
        return cw.toByteArray();
    }

    public static void main(String[] args) throws Exception {
        if (args.length != 2) {
            System.err.println("Usage: AndroidJarPatcher <desktop-pokewilds.jar> <android-core.jar>");
            System.exit(2);
        }
        Path in = Paths.get(args[0]);
        Path out = Paths.get(args[1]);
        Files.createDirectories(out.toAbsolutePath().getParent());
        Set<String> written = new HashSet<>();

        try (JarFile jf = new JarFile(in.toFile());
             JarOutputStream jos = new JarOutputStream(Files.newOutputStream(out))) {
            Enumeration<JarEntry> entries = jf.entries();
            while (entries.hasMoreElements()) {
                JarEntry e = entries.nextElement();
                String n = e.getName();
                if (e.isDirectory() || excluded(n) || !n.endsWith(".class")) continue;
                if (!written.add(n)) continue;
                byte[] data;
                try (InputStream is = jf.getInputStream(e)) {
                    data = is.readAllBytes();
                }
                data = patchClass(n, data);
                JarEntry ne = new JarEntry(n);
                ne.setTime(0L);
                jos.putNextEntry(ne);
                jos.write(data);
                jos.closeEntry();
            }
        }
        System.out.println("Wrote " + out + " (" + Files.size(out) + " bytes)");
    }
}
