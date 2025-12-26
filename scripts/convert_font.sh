#!/bin/bash
#
# LVGL Font Converter Script with Fallback
#
# Author: Colin Bitterfield
# Email: colin@bitterfield.com
# Date Created: 2025-12-23
# Version: 1.0.0
#
# Usage:
#   ./convert_font.sh FontName-Regular.ttf
#   ./convert_font.sh FontName-Regular.ttf 16 24 32
#

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FONTS_DIR="${PROJECT_ROOT}/main/fonts"
DEFAULT_SIZES=(16 18 20 24 32 48)
DEFAULT_RANGE="0x20-0x7F,0xB0"  # ASCII + degree symbol
BPP=4  # 4 bits per pixel for good quality

# Functions
print_header() {
    echo -e "${BLUE}================================================${NC}"
    echo -e "${BLUE}  LVGL Font Converter with Fallback${NC}"
    echo -e "${BLUE}================================================${NC}"
    echo ""
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ $1${NC}"
}

check_node() {
    if ! command -v node &> /dev/null; then
        print_error "Node.js is not installed!"
        echo ""
        echo "Please install Node.js first:"
        echo "  macOS:   brew install node"
        echo "  Linux:   sudo apt install nodejs npm"
        echo "  Windows: Download from https://nodejs.org"
        exit 1
    fi

    NODE_VERSION=$(node --version)
    print_success "Node.js found: $NODE_VERSION"
}

check_npm() {
    if ! command -v npm &> /dev/null; then
        print_error "npm is not installed!"
        exit 1
    fi
    print_success "npm found: $(npm --version)"
}

install_lv_font_conv() {
    print_info "Installing lv_font_conv..."

    # Try global install first
    if npm install -g lv_font_conv 2>/dev/null; then
        print_success "lv_font_conv installed globally"
        return 0
    fi

    # If global install fails, try local install
    print_warning "Global install failed, trying local install..."

    if npm install lv_font_conv 2>/dev/null; then
        print_success "lv_font_conv installed locally"
        # Create local wrapper
        cat > "${PROJECT_ROOT}/scripts/lv_font_conv_local.sh" <<'EOF'
#!/bin/bash
npx lv_font_conv "$@"
EOF
        chmod +x "${PROJECT_ROOT}/scripts/lv_font_conv_local.sh"
        LV_FONT_CONV_CMD="${PROJECT_ROOT}/scripts/lv_font_conv_local.sh"
        return 0
    fi

    print_error "Failed to install lv_font_conv"
    return 1
}

check_lv_font_conv() {
    print_info "Checking for lv_font_conv..."

    # Check global installation
    if command -v lv_font_conv &> /dev/null; then
        LV_FONT_CONV_CMD="lv_font_conv"
        print_success "lv_font_conv found (global)"
        return 0
    fi

    # Check local installation
    if [ -f "${PROJECT_ROOT}/node_modules/.bin/lv_font_conv" ]; then
        LV_FONT_CONV_CMD="npx lv_font_conv"
        print_success "lv_font_conv found (local)"
        return 0
    fi

    # Check for local wrapper
    if [ -f "${PROJECT_ROOT}/scripts/lv_font_conv_local.sh" ]; then
        LV_FONT_CONV_CMD="${PROJECT_ROOT}/scripts/lv_font_conv_local.sh"
        print_success "lv_font_conv found (local wrapper)"
        return 0
    fi

    # Not found - offer to install
    print_warning "lv_font_conv not found"
    echo ""
    echo "Would you like to install it now? (y/n)"
    read -r response

    if [[ "$response" =~ ^[Yy]$ ]]; then
        install_lv_font_conv
    else
        print_error "lv_font_conv is required. Exiting."
        exit 1
    fi
}

validate_font_file() {
    local font_file="$1"

    if [ ! -f "$font_file" ]; then
        print_error "Font file not found: $font_file"
        echo ""
        echo "Please provide a valid TTF/OTF file path"
        exit 1
    fi

    # Check file extension
    case "$(echo ${font_file} | tr A-Z a-z)" in
        *.ttf|*.otf|*.woff|*.woff2)
            print_success "Font file validated: $(basename "$font_file")"
            ;;
        *)
            print_error "Invalid font file type. Must be TTF, OTF, WOFF, or WOFF2"
            exit 1
            ;;
    esac
}

extract_font_name() {
    local font_file="$1"
    local basename=$(basename "$font_file")
    # Remove extension and convert to lowercase
    local font_name="${basename%.*}"
    font_name="$(echo ${font_name} | tr 'A-Z' 'a-z')"  # Convert to lowercase
    font_name="$(echo ${font_name} | tr ' ' '_')"  # Replace spaces with underscores
    font_name="$(echo ${font_name} | tr '-' '_')"  # Replace hyphens with underscores
    echo "$font_name"
}

convert_font_size() {
    local font_file="$1"
    local font_name="$2"
    local size="$3"
    local output_file="${FONTS_DIR}/${font_name}_${size}.c"

    print_info "Converting ${font_name} ${size}px..."

    # Create fonts directory if it doesn't exist
    mkdir -p "$FONTS_DIR"

    # Run conversion with error handling
    if $LV_FONT_CONV_CMD \
        --font "$font_file" \
        --size "$size" \
        --bpp "$BPP" \
        --format lvgl \
        --range "$DEFAULT_RANGE" \
        --no-compress \
        -o "$output_file" 2>&1; then

        # Check if file was created
        if [ -f "$output_file" ]; then
            local file_size=$(du -h "$output_file" | cut -f1)
            print_success "Created ${font_name}_${size}.c (${file_size})"
            return 0
        else
            print_error "Conversion failed - output file not created"
            return 1
        fi
    else
        print_error "Conversion command failed for size ${size}px"
        return 1
    fi
}

create_header_file() {
    local font_name="$1"
    shift
    local sizes=("$@")
    local header_file="${FONTS_DIR}/custom_fonts.h"

    print_info "Creating header file..."

    # Start header file
    cat > "$header_file" <<EOF
/**
 * Custom Fonts Header
 *
 * Auto-generated by convert_font.sh
 * Date: $(date '+%Y-%m-%d %H:%M:%S')
 */

#ifndef CUSTOM_FONTS_H
#define CUSTOM_FONTS_H

#include "lvgl.h"

// Font: ${font_name}
EOF

    # Add declarations for each size
    for size in "${sizes[@]}"; do
        echo "LV_FONT_DECLARE(${font_name}_${size});" >> "$header_file"
    done

    # Close header
    cat >> "$header_file" <<EOF

#endif // CUSTOM_FONTS_H
EOF

    print_success "Created custom_fonts.h"
}

update_cmake() {
    local font_name="$1"
    shift
    local sizes=("$@")

    print_info "Updating CMakeLists.txt..."

    local cmake_file="${PROJECT_ROOT}/main/CMakeLists.txt"
    local backup_file="${cmake_file}.bak.$(date +%Y%m%d_%H%M%S)"

    # Create backup
    cp "$cmake_file" "$backup_file"
    print_info "Backup created: $(basename "$backup_file")"

    # Check if fonts are already in CMakeLists.txt
    if grep -q "fonts/${font_name}" "$cmake_file"; then
        print_warning "Font entries already exist in CMakeLists.txt"
        echo "Please manually verify the entries are correct"
        return 0
    fi

    # Show what needs to be added
    echo ""
    print_warning "Please add these lines to main/CMakeLists.txt SRCS section:"
    echo ""
    for size in "${sizes[@]}"; do
        echo "        \"fonts/${font_name}_${size}.c\""
    done
    echo ""
}

print_usage_instructions() {
    local font_name="$1"
    shift
    local sizes=("$@")

    echo ""
    print_header
    print_success "Font conversion complete!"
    echo ""
    echo "Created files:"
    for size in "${sizes[@]}"; do
        echo "  - main/fonts/${font_name}_${size}.c"
    done
    echo "  - main/fonts/custom_fonts.h"
    echo ""

    echo "Next steps:"
    echo ""
    echo "1. Update main/CMakeLists.txt:"
    echo "   Add to SRCS section:"
    for size in "${sizes[@]}"; do
        echo "        \"fonts/${font_name}_${size}.c\""
    done
    echo ""

    echo "2. Include in your code:"
    echo "   #include \"fonts/custom_fonts.h\""
    echo ""

    echo "3. Use the fonts:"
    for size in "${sizes[@]}"; do
        echo "   lv_obj_set_style_text_font(label, &${font_name}_${size}, 0);"
    done
    echo ""

    echo "4. Rebuild project in CLion"
    echo ""
}

# Main script
main() {
    print_header

    # Check arguments
    if [ $# -lt 1 ]; then
        echo "Usage: $0 <font-file.ttf> [size1 size2 ...]"
        echo ""
        echo "Examples:"
        echo "  $0 Roboto-Regular.ttf"
        echo "  $0 Roboto-Regular.ttf 16 24 32"
        echo "  $0 ~/Downloads/OpenSans-Regular.ttf"
        echo ""
        exit 1
    fi

    FONT_FILE="$1"
    shift

    # Get sizes (use defaults if not specified)
    if [ $# -gt 0 ]; then
        SIZES=("$@")
    else
        SIZES=("${DEFAULT_SIZES[@]}")
    fi

    # Validate environment
    check_node
    check_npm
    check_lv_font_conv

    echo ""

    # Validate font file
    validate_font_file "$FONT_FILE"

    # Extract font name
    FONT_NAME=$(extract_font_name "$FONT_FILE")
    print_info "Font name: $FONT_NAME"

    echo ""
    print_info "Converting sizes: ${SIZES[*]}"
    print_info "Character range: $DEFAULT_RANGE"
    print_info "Bits per pixel: $BPP"
    echo ""

    # Convert each size
    CONVERTED_SIZES=()
    FAILED_SIZES=()

    for size in "${SIZES[@]}"; do
        if convert_font_size "$FONT_FILE" "$FONT_NAME" "$size"; then
            CONVERTED_SIZES+=("$size")
        else
            FAILED_SIZES+=("$size")
        fi
    done

    echo ""

    # Create header file if any conversions succeeded
    if [ ${#CONVERTED_SIZES[@]} -gt 0 ]; then
        create_header_file "$FONT_NAME" "${CONVERTED_SIZES[@]}"
        update_cmake "$FONT_NAME" "${CONVERTED_SIZES[@]}"
        print_usage_instructions "$FONT_NAME" "${CONVERTED_SIZES[@]}"
    fi

    # Report failures
    if [ ${#FAILED_SIZES[@]} -gt 0 ]; then
        echo ""
        print_error "Failed to convert sizes: ${FAILED_SIZES[*]}"
    fi

    # Summary
    echo ""
    print_info "Conversion complete!"
    print_success "Converted: ${#CONVERTED_SIZES[@]} sizes"
    if [ ${#FAILED_SIZES[@]} -gt 0 ]; then
        print_error "Failed: ${#FAILED_SIZES[@]} sizes"
    fi
}

# Run main function
main "$@"
