import org.gbarecomp.SaveArchive;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

public final class AndroidSaveArchiveTest {
    private static void reject(Path root, String label) throws Exception {
        try {
            SaveArchive.write(root.toFile(), new ByteArrayOutputStream());
        } catch (IOException expected) {
            System.out.println("PASS rejected: " + label);
            return;
        }
        throw new AssertionError("Accepted " + label);
    }

    public static void main(String[] args) throws Exception {
        Path parent = Path.of(args[0]);
        Files.createDirectories(parent);
        Path empty = Files.createTempDirectory(parent, "empty-");
        reject(empty, "empty storage");
        Path invalid = Files.createTempDirectory(parent, "invalid-");
        Files.createDirectories(invalid.resolve("saves"));
        Files.write(invalid.resolve("saves/SacredStonesRecomp.sav"), new byte[1]);
        reject(invalid, "incorrect SRAM size");
        Path root = Files.createTempDirectory(parent, "valid-");
        Files.createDirectories(root.resolve("saves"));
        Files.createDirectories(root.resolve("roms"));
        Files.createDirectories(root.resolve("bios"));
        byte[] save = new byte[32768];
        Arrays.fill(save, (byte) 0x5a);
        Path battery = root.resolve("saves/SacredStonesRecomp.sav");
        Files.write(battery, save);
        byte[] state = new byte[] {1, 2, 3, 4};
        Files.write(root.resolve("roms/sacred_stones_usa.state1"), state);
        Files.write(root.resolve("roms/sacred_stones_usa.suspend.state"), state);
        Files.write(root.resolve("roms/sacred_stones_usa.gba"), new byte[] {9});
        Files.write(root.resolve("bios/gba_bios.bin"), new byte[] {9});
        Files.write(root.resolve("saves/unrelated.sav"), new byte[] {9});
        Files.write(root.resolve("roms/sacred_stones_usa.state10"), new byte[] {9});
        long originalTime = Files.getLastModifiedTime(battery).toMillis();
        ByteArrayOutputStream output = new ByteArrayOutputStream();
        if (SaveArchive.write(root.toFile(), output) != 3) throw new AssertionError("Wrong file count");
        Map<String, byte[]> entries = new HashMap<>();
        try (ZipInputStream zip = new ZipInputStream(new ByteArrayInputStream(output.toByteArray()))) {
            ZipEntry entry;
            while ((entry = zip.getNextEntry()) != null) entries.put(entry.getName(), zip.readAllBytes());
        }
        if (entries.size() != 4 || !Arrays.equals(entries.get("saves/SacredStonesRecomp.sav"), save) ||
            !Arrays.equals(entries.get("roms/sacred_stones_usa.state1"), state) ||
            !Arrays.equals(entries.get("roms/sacred_stones_usa.suspend.state"), state) ||
            !entries.containsKey("backup.json")) throw new AssertionError("Unexpected archive content");
        if (!Arrays.equals(Files.readAllBytes(battery), save) ||
            Files.getLastModifiedTime(battery).toMillis() != originalTime)
            throw new AssertionError("Export modified original save");
        System.out.println("PASS: exact save/state bytes, manifest, ROM/BIOS exclusion, originals unchanged");
        Files.delete(battery);
        output.reset();
        if (SaveArchive.write(root.toFile(), output) != 2) throw new AssertionError("States-only export failed");
        System.out.println("PASS: save-state-only export");
        Files.write(root.resolve("roms/sacred_stones_usa.state2"), new byte[0]);
        reject(root, "empty save state");
        try (java.io.RandomAccessFile large = new java.io.RandomAccessFile(
                root.resolve("roms/sacred_stones_usa.state2").toFile(), "rw")) {
            large.setLength(16 * 1024 * 1024 + 1);
        }
        reject(root, "oversized save state");
        Files.delete(root.resolve("roms/sacred_stones_usa.state2"));
        try {
            SaveArchive.write(root.toFile(), new java.io.OutputStream() {
                public void write(int value) throws IOException { throw new IOException("Storage full"); }
            });
            throw new AssertionError("Write failure ignored");
        } catch (IOException expected) {
            if (!Arrays.equals(Files.readAllBytes(root.resolve("roms/sacred_stones_usa.state1")), state))
                throw new AssertionError("Failed export modified original state");
            System.out.println("PASS: write failure leaves original state unchanged");
        }
    }
}
