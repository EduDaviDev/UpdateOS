import sys
import json
import subprocess
from pathlib import Path

from PyQt6.QtWidgets import QApplication, QMainWindow, QMessageBox, QTreeWidgetItem
from gui_ui import Ui_MainWindow


# ============================================================
#  Paths
# ============================================================

THIS_DIR = Path(__file__).resolve()
BASE_DIR = THIS_DIR.parents[4]                 # UpOS/

ASSETS_DIR      = BASE_DIR / "assets"
TOOLS_DIR       = ASSETS_DIR / "Tools"
TOOLS_CODES_DIR = TOOLS_DIR / "Codes"
TOOLS_BIN_DIR   = TOOLS_DIR / "Binaries"

APPS_DIR        = ASSETS_DIR / "Apps"
APPS_CODES_DIR  = APPS_DIR / "Codes"
APPS_BIN_DIR    = APPS_DIR / "Binaries"

BITMAPS_DIR         = ASSETS_DIR / "Bitmaps"
BITMAPS_SOURCES_DIR = BITMAPS_DIR / "Sources"
BITMAPS_HEADERS_DIR = BITMAPS_DIR / "Headers"

FONTS_DIR        = ASSETS_DIR / "Fonts"
FONTS_BASES_DIR  = FONTS_DIR / "Bases"
FONTS_OUT_DIR    = FONTS_DIR / "Out"
FONTS_BIN_DIR    = FONTS_OUT_DIR / "Bin"
FONTS_HEADERS_DIR = FONTS_OUT_DIR / "Headers"

DISKS_DIR      = ASSETS_DIR / "Disks"
DISKS_DATA_DIR = DISKS_DIR / "Data"
DISKS_IMGS_DIR = DISKS_DIR / "Images"

# Import img2all (sibling of MakeGUI)
sys.path.insert(0, str(TOOLS_CODES_DIR))
from img2all import convert_image_to_font_h, convert_image_to_bmp


APP_EXTENSIONS = [".c", ".cpp", ".c++", ".asm", ".s"]

DISK_JSON_PATH            = DISKS_DIR / "Disks.json"
SAME_DISK_NAME_COUNT_PATH = DISKS_DIR / "same_disk_name_count.json"

NEW_DISK_TEMPLATE = {
    "name": "New Disk",
    "size": 128,
    "fs":   "FAT32",
    "dir":  "NewDisk",
}


# ============================================================
#  Ensure all expected folders exist
# ============================================================

for _d in (
    APPS_CODES_DIR, APPS_BIN_DIR,
    BITMAPS_SOURCES_DIR, BITMAPS_HEADERS_DIR,
    FONTS_BASES_DIR, FONTS_OUT_DIR, FONTS_BIN_DIR, FONTS_HEADERS_DIR,
    DISKS_DATA_DIR, DISKS_IMGS_DIR,
):
    _d.mkdir(parents=True, exist_ok=True)


# ============================================================
#  JSON helpers
# ============================================================

def load_json(path, default=None):
    if default is None:
        default = []
    try:
        with open(path, "r", encoding="utf-8") as f:
            return json.load(f)
    except FileNotFoundError:
        print(f"[warning] {path} not found. Using default.")
        return default
    except json.JSONDecodeError as e:
        print(f"[error] {path} is invalid ({e.lineno}:{e.msg}). Using default.")
        return default


def save_json(path, obj):
    Path(path).parent.mkdir(parents=True, exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        json.dump(obj, f, indent=4, ensure_ascii=False)


# ============================================================
#  Global state
# ============================================================

disk_json_obj = load_json(DISK_JSON_PATH, default=[])

# Same-name counters, split by category:
#   { "Names": { "New Disk": 3, ... }, "Dirs": { "NewDisk": 3, ... } }
same_disk_name_count = load_json(SAME_DISK_NAME_COUNT_PATH,
                                 default={"Names": {}, "Dirs": {}})
if not isinstance(same_disk_name_count, dict):
    same_disk_name_count = {"Names": {}, "Dirs": {}}
else:
    same_disk_name_count.setdefault("Names", {})
    same_disk_name_count.setdefault("Dirs", {})


selected_disk = None
selected_app  = None
selected_font = None
selected_bmp  = None


# ============================================================
#  Main window
# ============================================================

class JanelaPrincipal(QMainWindow, Ui_MainWindow):
    def __init__(self):
        super().__init__()
        self.setupUi(self)

        # Disabled widgets: gray background and text (borders untouched)
        self.setStyleSheet("""
            QLineEdit:disabled, QComboBox:disabled, QPushButton:disabled {
                background-color: #e8e8e8;
                color: #a0a0a0;
            }
        """)

        # --- Build ---
        self.btn_CompileRun.clicked.connect(self.f_compile_run)
        self.btn_Compile.clicked.connect(self.f_compile_only)
        self.btn_Run.clicked.connect(self.f_run_only)
        self.btn_Clear.clicked.connect(self.f_clear_build_files)

        # --- Disks ---
        self.btn_build_disk_img.clicked.connect(self.f_build_disk_img)
        self.btn_build_all_img.clicked.connect(self.f_build_all_disk_imgs)
        self.btn_reload_disks.clicked.connect(self.f_reload_disks)
        self.btn_add_disk.clicked.connect(self.f_add_disk)
        self.btn_remove_disk.clicked.connect(self.f_remove_disk)
        self.btn_remove_all_disks.clicked.connect(self.f_remove_all_disks)
        self.btn_save_disk.clicked.connect(self.f_save_disk)

        # --- Apps ---
        self.btn_build_app.clicked.connect(self.f_build_app)
        self.btn_build_all_apps.clicked.connect(self.f_build_all_apps)
        self.btn_reload_apps.clicked.connect(self.f_reload_apps)

        # --- Bitmaps ---
        self.btn_convert_bmp.clicked.connect(self.f_convert_bmp)
        self.btn_convert_all_bmp.clicked.connect(self.f_convert_all_bmp)
        self.btn_reload_bitmaps.clicked.connect(self.f_reload_bmps)

        # --- Fonts ---
        self.btn_convert_font.clicked.connect(self.f_convert_font)
        self.btn_convert_all_fonts.clicked.connect(self.f_convert_all_fonts)
        self.btn_reload_fonts.clicked.connect(self.f_reload_fonts)

        # --- Selection ---
        self.disk_list.currentItemChanged.connect(self.when_disk_changed)
        self.app_list.currentItemChanged.connect(self.when_app_changed)
        self.font_pictures.currentItemChanged.connect(self.when_font_changed)
        self.bmp_files.currentItemChanged.connect(self.when_bmp_changed)

        # Start with disk fields disabled
        self._set_disk_fields_enabled(False)

    # ============================================================
    #  Build system
    # ============================================================

    def f_compile_run(self):
        self.f_compile_only()
        self.f_run_only()

    def f_compile_only(self):
        subprocess.run(["make", "system"], cwd=BASE_DIR)

    def f_run_only(self):
        subprocess.run(["make", "run"], cwd=BASE_DIR)

    def f_clear_build_files(self):
        subprocess.run(["make", "clean"], cwd=BASE_DIR)

    # ============================================================
    #  Persistence
    # ============================================================

    def save_all(self):
        """Save every persistent state of the application at once."""
        save_json(DISK_JSON_PATH, disk_json_obj)
        save_json(SAME_DISK_NAME_COUNT_PATH, same_disk_name_count)

    def closeEvent(self, event):
        """Save everything before closing the app."""
        try:
            self.save_all()
        except Exception as e:
            print(f"[error] Failed to save on close: {e}")
        event.accept()

    # ============================================================
    #  Disks
    # ============================================================

    def _set_disk_fields_enabled(self, enabled):
        """Enable or disable all disk-related fields at once."""
        self.disk_name.setEnabled(enabled)
        self.disk_size.setEnabled(enabled)
        self.fses.setEnabled(enabled)
        self.disk_dir.setEnabled(enabled)
        self.btn_save_disk.setEnabled(enabled)
        self.btn_remove_disk.setEnabled(enabled)
        self.btn_build_disk_img.setEnabled(enabled)

    def f_reload_disks(self):
        global disk_json_obj

        disk_json_obj = load_json(DISK_JSON_PATH, default=[])

        self.disk_list.blockSignals(True)
        self.disk_list.clear()
        for disk in disk_json_obj:
            self.disk_list.addItem(disk["name"])
        self.disk_list.blockSignals(False)

        self._set_disk_fields_enabled(False)
        self.btn_remove_all_disks.setEnabled(bool(disk_json_obj))

    def find_disk_by_name(self, name):
        for disk in disk_json_obj:
            if disk["name"] == name:
                return disk
        return None

    def make_unique_name(self, base_name, field="name", exclude=None):
        """
        Return a unique value for the given field.

        `field` is the JSON key of the disk: "name" or "dir".
        Counters are stored under "Names" (for "name") or "Dirs" (for "dir").

        If `exclude` is provided, that disk is ignored during the collision
        check — useful when renaming a disk that already exists.
        """
        global same_disk_name_count

        category = "Names" if field == "name" else "Dirs"
        counters = same_disk_name_count[category]

        existing = {d[field] for d in disk_json_obj if d is not exclude}

        if base_name not in existing:
            counters.setdefault(base_name, 1)
            return base_name

        n = counters.get(base_name, 1) + 1
        while f"{base_name}_{n}" in existing:
            n += 1

        counters[base_name] = n
        return f"{base_name}_{n}"

    def when_disk_changed(self, current, previous):
        global selected_disk

        if current is None:
            selected_disk = None
            self._set_disk_fields_enabled(False)
            return

        selected_disk = current.text()
        disk = self.find_disk_by_name(selected_disk)
        if disk is None:
            self._set_disk_fields_enabled(False)
            return

        self.disk_name.setText(disk["name"])
        self.disk_size.setText(str(disk["size"]))
        self.fses.setCurrentText(disk["fs"])
        self.disk_dir.setText(disk["dir"])

        self._set_disk_fields_enabled(True)

    def f_save_disk(self):
        global disk_json_obj, selected_disk

        if selected_disk is None:
            QMessageBox.warning(self, "Warning", "No disk selected.")
            return

        disk = self.find_disk_by_name(selected_disk)
        if disk is None:
            QMessageBox.warning(self, "Warning", "Disk not found.")
            return

        new_name = self.disk_name.text().strip()
        new_dir  = self.disk_dir.text().strip()
        new_fs   = self.fses.currentText()

        if not new_name:
            QMessageBox.warning(self, "Warning", "Name cannot be empty.")
            return
        if not new_dir:
            QMessageBox.warning(self, "Warning", "Directory cannot be empty.")
            return

        try:
            new_size = int(self.disk_size.text())
        except ValueError:
            QMessageBox.warning(self, "Warning", "Size must be an integer.")
            return

        old_name = disk["name"]
        old_dir  = disk["dir"]

        if new_name != old_name:
            new_name = self.make_unique_name(new_name, field="name", exclude=disk)
        if new_dir != old_dir:
            new_dir = self.make_unique_name(new_dir, field="dir", exclude=disk)

        disk["name"] = new_name
        disk["size"] = new_size
        disk["fs"]   = new_fs
        disk["dir"]  = new_dir

        # Rename the physical data folder if the dir changed
        if new_dir != old_dir:
            old_path = DISKS_DATA_DIR / old_dir
            new_path = DISKS_DATA_DIR / new_dir
            if old_path.is_dir() and not new_path.exists():
                try:
                    old_path.rename(new_path)
                except OSError as e:
                    print(f"[warning] Could not rename directory: {e}")

        # If the name changed, rename the image file too (if it exists)
        if new_name != old_name:
            old_img = DISKS_IMGS_DIR / f"{old_name}.img"
            new_img = DISKS_IMGS_DIR / f"{new_name}.img"
            if old_img.is_file() and not new_img.exists():
                try:
                    old_img.rename(new_img)
                except OSError as e:
                    print(f"[warning] Could not rename image: {e}")

        selected_disk = new_name

        self.save_all()
        QMessageBox.information(self, "Success", "Disk saved.")
        self.f_reload_disks()

    def f_add_disk(self):
        global disk_json_obj, selected_disk

        disk = NEW_DISK_TEMPLATE.copy()

        disk["name"] = self.make_unique_name(disk["name"], field="name")
        disk["dir"]  = self.make_unique_name(disk["dir"],  field="dir")

        # Data folder: assets/Disks/Data/<dir>/
        (DISKS_DATA_DIR / disk["dir"]).mkdir(parents=True, exist_ok=True)

        selected_disk = disk["name"]
        disk_json_obj.append(disk)

        self.save_all()
        QMessageBox.information(self, "Success", "Disk added.")
        self.f_reload_disks()

    def f_remove_disk(self):
        global disk_json_obj, selected_disk

        if selected_disk is None:
            QMessageBox.warning(self, "Warning", "No disk selected to remove.")
            return

        disk = self.find_disk_by_name(selected_disk)

        disk_json_obj = [d for d in disk_json_obj if d["name"] != selected_disk]

        # Also remove the data folder and the image (if they exist)
        if disk is not None:
            data_dir = DISKS_DATA_DIR / disk["dir"]
            if data_dir.is_dir():
                import shutil
                shutil.rmtree(data_dir, ignore_errors=True)

            img = DISKS_IMGS_DIR / f"{disk['name']}.img"
            if img.is_file():
                img.unlink(missing_ok=True)

        selected_disk = None
        self.save_all()
        QMessageBox.information(self, "Success", "Disk removed.")
        self.f_reload_disks()

    def f_remove_all_disks(self):
        global disk_json_obj, selected_disk

        if not disk_json_obj:
            QMessageBox.warning(self, "Warning", "No disks to remove.")
            return

        reply = QMessageBox.question(
            self, "Confirm",
            f"Remove all {len(disk_json_obj)} disk(s)?\n"
            "Their data folders and image files will also be deleted.",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.No,
        )
        if reply != QMessageBox.StandardButton.Yes:
            return

        import shutil
        for disk in disk_json_obj:
            data_dir = DISKS_DATA_DIR / disk["dir"]
            if data_dir.is_dir():
                shutil.rmtree(data_dir, ignore_errors=True)

            img = DISKS_IMGS_DIR / f"{disk['name']}.img"
            if img.is_file():
                img.unlink(missing_ok=True)

        disk_json_obj = []
        selected_disk = None

        self.save_all()
        QMessageBox.information(self, "Success", "All disks removed.")
        self.f_reload_disks()

    def f_build_disk_img(self):
        disk = self.find_disk_by_name(selected_disk) if selected_disk else None
        if disk is None:
            QMessageBox.warning(self, "Warning", "No disk selected to build.")
            return

        subprocess.run([
            "make", "disk",
            f"DISK_NAME={disk['name']}",
            f"DISK_SIZE={disk['size']}",
            f"DISK_FS={disk['fs']}",
            f"DISK_DIR={disk['dir']}",
            f"DISK_DATA={DISKS_DATA_DIR}",
            f"DISK_OUT={DISKS_IMGS_DIR / (disk['name'] + '.img')}",
        ], cwd=BASE_DIR)

    def f_build_all_disk_imgs(self):
        for disk in disk_json_obj:
            subprocess.run([
                "make", "disk",
                f"DISK_NAME={disk['name']}",
                f"DISK_SIZE={disk['size']}",
                f"DISK_FS={disk['fs']}",
                f"DISK_DIR={disk['dir']}",
                f"DISK_DATA={DISKS_DATA_DIR}",
                f"DISK_OUT={DISKS_IMGS_DIR / (disk['name'] + '.img')}",
            ], cwd=BASE_DIR)

    # ============================================================
    #  Apps
    # ============================================================

    def f_reload_apps(self):
        self.app_list.clear()
        if not APPS_CODES_DIR.is_dir():
            return
        for item in APPS_CODES_DIR.iterdir():
            if item.is_dir() or item.suffix.lower() in APP_EXTENSIONS:
                self.app_list.addItem(item.name)

    def when_app_changed(self, current, previous):
        global selected_app
        selected_app = current.text() if current else None

    def f_build_app(self):
        if not selected_app:
            QMessageBox.warning(self, "Warning", "No application selected to build.")
            return

        subprocess.run([
            f"{TOOLS_CODES_DIR}/makeapps.sh",
            selected_app
        ], cwd=BASE_DIR)

    def f_build_all_apps(self):
        if not APPS_CODES_DIR.is_dir():
            return
        for app in APPS_CODES_DIR.iterdir():
            if app.is_dir() or app.suffix.lower() in APP_EXTENSIONS:
                subprocess.run([
                    f"{TOOLS_CODES_DIR}/makeapps.sh",
                    app.name
                ], cwd=BASE_DIR)

    # ============================================================
    #  Fonts
    # ============================================================

    def f_reload_fonts(self):
        self.font_pictures.clear()
        if not FONTS_BASES_DIR.is_dir():
            return
        for item in FONTS_BASES_DIR.iterdir():
            if item.is_dir():
                self.font_pictures.addTopLevelItem(QTreeWidgetItem([item.name]))

    def when_font_changed(self, current, previous):
        global selected_font
        selected_font = current.text(0) if current else None

    def _convert_one_font(self, font_name):
        json_path = FONTS_BASES_DIR / font_name / "font.json"
        font_data = load_json(json_path, default=None)
        if not font_data:
            return

        FONTS_HEADERS_DIR.mkdir(parents=True, exist_ok=True)

        # NOTE: adjust parameter names to match the real
        # signature of convert_image_to_font_h.
        convert_image_to_font_h(
            name        = font_data["name"],
            author      = font_data["author"],
            image       = FONTS_BASES_DIR / font_name / font_data["image"],
            char_width  = font_data["char_width"],
            char_height = font_data["char_height"],
            char_count  = font_data["char_count"],
            first_char  = font_data["first_char"],
            last_char   = font_data["last_char"],
            flags       = font_data["flags"],
            output      = FONTS_HEADERS_DIR / f"{font_name}.h",
        )

    def f_convert_font(self):
        if not selected_font:
            QMessageBox.warning(self, "Warning", "No font selected.")
            return
        self._convert_one_font(selected_font)

    def f_convert_all_fonts(self):
        if not FONTS_BASES_DIR.is_dir():
            return
        for font in FONTS_BASES_DIR.iterdir():
            if font.is_dir():
                self._convert_one_font(font.name)

    # ============================================================
    #  Bitmaps
    # ============================================================

    def f_reload_bmps(self):
        self.bmp_files.clear()
        if not BITMAPS_SOURCES_DIR.is_dir():
            return
        for item in BITMAPS_SOURCES_DIR.iterdir():
            self.bmp_files.addTopLevelItem(QTreeWidgetItem([item.name]))

    def when_bmp_changed(self, current, previous):
        global selected_bmp
        selected_bmp = current.text(0) if current else None

    def f_convert_bmp(self):
        if not selected_bmp:
            QMessageBox.warning(self, "Warning", "No bitmap selected.")
            return

        BITMAPS_HEADERS_DIR.mkdir(parents=True, exist_ok=True)

        stem = Path(selected_bmp).stem
        convert_image_to_bmp(
            name   = stem,
            image  = BITMAPS_SOURCES_DIR / selected_bmp,
            output = BITMAPS_HEADERS_DIR / f"{stem}.h",
        )

    def f_convert_all_bmp(self):
        BITMAPS_HEADERS_DIR.mkdir(parents=True, exist_ok=True)

        if not BITMAPS_SOURCES_DIR.is_dir():
            return

        for bmp in BITMAPS_SOURCES_DIR.iterdir():
            if not bmp.is_file():
                continue
            convert_image_to_bmp(
                name   = bmp.stem,
                image  = bmp,
                output = BITMAPS_HEADERS_DIR / f"{bmp.stem}.h",
            )


# ============================================================
#  Entry point
# ============================================================

if __name__ == "__main__":
    app = QApplication(sys.argv)
    janela = JanelaPrincipal()

    janela.f_reload_disks()
    janela.f_reload_apps()
    janela.f_reload_fonts()
    janela.f_reload_bmps()

    janela.show()
    sys.exit(app.exec())