#!/bin/bash

echo "=== Step 1: 安装 git-filter-repo ==="
if ! command -v git-filter-repo &> /dev/null
then
    echo "git-filter-repo 未安装，开始下载..."
    wget https://raw.githubusercontent.com/newren/git-filter-repo/main/git-filter-repo -O /tmp/git-filter-repo
    chmod +x /tmp/git-filter-repo
    sudo mv /tmp/git-filter-repo /usr/local/bin/
else
    echo "git-filter-repo 已安装。"
fi

echo "=== Step 2: 检查是否在 Git 仓库中 ==="
if [ ! -d ".git" ]; then
    echo "错误：当前目录不是 Git 仓库，请 cd 到仓库根目录后再运行本脚本。"
    exit 1
fi

echo "=== Step 3: 重写历史，删除所有 build/ ==="
git filter-repo --path build/ --invert-paths

echo "=== Step 4: Git 垃圾回收优化 ==="
git reflog expire --expire=now --all
git gc --prune=now --aggressive

echo "=== Step 5: 强制推送到 GitHub（重写远程历史） ==="
branch=$(git rev-parse --abbrev-ref HEAD)
git push origin $branch --force

echo ""
echo "🎉 完成！build/ 已从所有历史中彻底删除，并已推送到 GitHub。"

