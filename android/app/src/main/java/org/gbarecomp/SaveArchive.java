package org.gbarecomp;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.file.Files;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

/** Read-only FE8 backup: deliberately excludes ROM, BIOS and arbitrary files. */
public final class SaveArchive {
    private static final String ROM = "sacred_stones_usa";
    private static final long MAX_STATE_SIZE = 16 * 1024 * 1024;
    private SaveArchive() {}

    private static List<File> collect(File root) throws IOException {
        List<File> files = new ArrayList<>();
        add(root, "saves/SacredStonesRecomp.sav", files, true);
        for (int slot = 1; slot <= 9; slot++)
            add(root, "roms/" + ROM + ".state" + slot, files, false);
        add(root, "roms/" + ROM + ".suspend.state", files, false);
        if (files.isEmpty()) throw new IOException("No saved games or save states to export.");
        return files;
    }

    private static void add(File root, String relative, List<File> files,
                            boolean battery) throws IOException {
        File file = new File(root, relative);
        if (!file.exists() && !Files.isSymbolicLink(file.toPath())) return;
        if (!file.getCanonicalFile().equals(file.getAbsoluteFile()) || !file.isFile())
            throw new IOException("Unsafe save path: " + relative);
        if (battery ? file.length() != 32768 : file.length() < 1 || file.length() > MAX_STATE_SIZE)
            throw new IOException("Unexpected save size: " + relative);
        files.add(file);
    }

    public static int write(File storage, OutputStream output) throws IOException {
        File root = storage.getCanonicalFile();
        List<File> files = collect(root);
        try (ZipOutputStream zip = new ZipOutputStream(output)) {
            zip.putNextEntry(new ZipEntry("backup.json"));
            zip.write(("{\"schema\":1,\"game\":\"SacredStonesRecomp\","
                + "\"romSha1\":\"c25b145e37456171ada4b0d440bf88a19f4d509f\"}\n")
                .getBytes(StandardCharsets.UTF_8));
            zip.closeEntry();
            byte[] buffer = new byte[65536];
            for (File file : files) {
                long size = file.length();
                long modified = file.lastModified();
                Object fileKey = Files.readAttributes(file.toPath(),
                    java.nio.file.attribute.BasicFileAttributes.class).fileKey();
                String relative = root.toPath().relativize(file.toPath()).toString().replace('\\', '/');
                zip.putNextEntry(new ZipEntry(relative));
                long copied = 0;
                try (FileInputStream input = new FileInputStream(file)) {
                    int count;
                    while ((count = input.read(buffer)) != -1) {
                        copied += count;
                        if (copied > size) throw new IOException("Save changed during export; try again.");
                        zip.write(buffer, 0, count);
                    }
                }
                Object currentKey = Files.readAttributes(file.toPath(),
                    java.nio.file.attribute.BasicFileAttributes.class).fileKey();
                if (copied != size || file.length() != size || file.lastModified() != modified ||
                    !java.util.Objects.equals(fileKey, currentKey))
                    throw new IOException("Save changed during export; try again.");
                zip.closeEntry();
            }
        }
        return files.size();
    }
}
