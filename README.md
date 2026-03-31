# Sonic

Windows 환경에서 Visual Studio로 개발한 DirectX 11 기반 소닉 프로젝트입니다.

![Visual Studio 2022](https://img.shields.io/badge/Visual%20Studio-2022-5C2D91?logo=visualstudio&logoColor=white)
![DirectX 11](https://img.shields.io/badge/DirectX-11-107C10?logo=windows&logoColor=white)
![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)

## 프로젝트 소개

이 저장소는 Sonic 테마의 Windows C++ 게임 클라이언트를 포함하고 있습니다.

주요 구성:

- `Dx11.sln`: Visual Studio 솔루션 파일
- `GameClient/`: 메인 클라이언트 소스 코드
- `Game/`: 텍스처, 셰이더, 스프라이트, 플립북, 타일맵 등 게임 리소스
- `CodeGen/`: 코드 생성 도구 소스
- `External/`: 외부 헤더 및 라이브러리

## 개발 환경

- Windows
- Visual Studio 2022
- MSBuild
- DirectX 11

## 빌드 방법

`Dx11.sln`을 Visual Studio에서 열고 `GameClient` 타깃을 빌드하면 됩니다.

명령줄에서 MSBuild로 빌드하려면:

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" "Dx11.sln" /t:GameClient /p:Configuration=Debug /p:Platform=x64 /m
```

## 실행 방법

빌드가 완료되면 `x64 Debug` 출력 폴더에 생성된 `GameClient` 실행 파일을 실행하면 됩니다.

## 스크린샷

### 타일맵 텍스처

스테이지를 구성하는 타일셋과 배치용 맵 텍스처 예시입니다. 지형, 충돌 구간, 배경 구성을 시각적으로 확인할 수 있습니다.

![Sonic Map](Game/Content/Texture/SonicMap.png)

### 캐릭터 텍스처

소닉 캐릭터 스프라이트 시트입니다. 이동, 대기, 점프, 스킬 같은 애니메이션 프레임 작업에 사용됩니다.

![Sonic Character](Game/Content/Texture/Sonic.png)

## 실행 화면 설명

현재 README에는 실제 플레이 캡처 대신 프로젝트에서 사용하는 핵심 리소스 이미지를 먼저 정리해두었습니다.

- `SonicMap.png`: 레벨 구성과 타일 배치를 확인할 수 있는 맵 텍스처
- `Sonic.png`: 캐릭터 애니메이션 제작에 사용하는 소닉 스프라이트 시트

실제 게임 실행 장면 캡처를 추가하면 프로젝트 목적과 완성도를 더 직관적으로 보여줄 수 있습니다.

## 저장소 메모

- `.gitignore`를 통해 `.vs/` 같은 Visual Studio 캐시 파일은 제외됩니다.
- 현재 저장소에는 프로젝트 실행에 필요한 에셋과 일부 바이너리 의존성이 포함되어 있습니다.

## 빠른 푸시 방법

변경사항을 빠르게 커밋하고 푸시할 수 있도록 `push.ps1` 스크립트를 포함했습니다.

기본 커밋 메시지로 푸시:

```powershell
.\push.ps1
```

커스텀 커밋 메시지로 푸시:

```powershell
.\push.ps1 "플레이어 이동 수정"
```

이 스크립트는 아래 작업을 자동으로 수행합니다.

1. 모든 변경사항 스테이징
2. 커밋 생성
3. `origin/main`으로 푸시

## GitHub

저장소 주소:

[https://github.com/CJH1022/Sonic](https://github.com/CJH1022/Sonic)
