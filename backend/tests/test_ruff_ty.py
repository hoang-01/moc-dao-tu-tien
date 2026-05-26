import subprocess
from pathlib import Path

BACKEND_ROOT = Path(__file__).parent.parent


def run_check(command: str):
    """Hàm bổ trợ để chạy lệnh và trả về kết quả."""
    result = subprocess.run(
        command, shell=True, capture_output=True, text=True, cwd=BACKEND_ROOT
    )
    assert result.returncode == 0, (
        f"Thất bại tại lệnh: {command}\n{result.stdout}\n{result.stderr}"
    )


def test_ruff_lint():
    """Kiểm tra lỗi linter bằng Ruff."""
    run_check("ruff check .")


def test_ruff_format():
    """Kiểm tra định dạng code bằng Ruff."""
    run_check("ruff format --check .")


def test_ty_type_check():
    """Kiểm tra kiểu dữ liệu bằng Ty."""
    run_check("ty check .")
