import Dependencies._
import com.eed3si9n.jarjarabrams.ShadeRule
import com.jsuereth.sbtpgp.PgpKeys.pgpPassphrase
import sbt.Keys._
import sbt._
import sbt.plugins.JvmPlugin
import sbtassembly.Assembly.{Library, Project}
import sbtassembly.AssemblyPlugin.autoImport._
import sbtassembly.MergeStrategy
import spyt.SparkForkVersion.sparkForkVersion
import spyt.SpytPlugin.autoImport._
import spyt.YtPublishPlugin

import java.nio.file.Paths

object CommonPlugin extends AutoPlugin {
  override def trigger = AllRequirements

  override def requires = JvmPlugin && YtPublishPlugin

  object autoImport {
    val commonShadeRules: Seq[ShadeRule] = {
      // TODO get names from arrow dependency
      // Preserve arrow classes from shading
      val arrowBuffers = Seq("ArrowBuf", "ExpandableByteBuf", "LargeBuffer", "MutableWrappedByteBuf",
        "NettyArrowBuf", "PooledByteBufAllocatorL", "UnsafeDirectLittleEndian")
      val arrowRules = arrowBuffers.map(s"io.netty.buffer." + _).map(n => ShadeRule.rename(n -> n).inAll)
      arrowRules ++ Seq(
        ShadeRule.rename("javax.annotation.**" -> "shadedspyt.javax.annotation.@1")
          .inLibrary("com.google.code.findbugs" % "annotations" % "2.0.3"),
        ShadeRule.zap("META-INF.org.apache.logging.log4j.core.config.plugins.Log4j2Plugins.dat")
          .inLibrary("org.apache.logging.log4j" % "log4j-core" % "2.11.0"),
        ShadeRule.rename("io.netty.**" -> "shadedspyt.io.netty.@1")
          .inAll
      )
    }

    val clusterShadeRules: Seq[ShadeRule] = commonShadeRules ++ Seq(ShadeRule.rename(
      "tech.ytsaurus.spark.log4j.AsyncLoggerHelper" -> "tech.ytsaurus.spark.log4j.AsyncLoggerHelper",
      "tech.ytsaurus.spyt.submit.*" -> "tech.ytsaurus.spyt.submit.@1",
      "tech.ytsaurus.spyt.fs.YtCachedFileSystem" -> "tech.ytsaurus.spyt.fs.YtCachedFileSystem",
      "tech.ytsaurus.spyt.fs.YtFileSystem" -> "tech.ytsaurus.spyt.fs.YtFileSystem",
      "tech.ytsaurus.spyt.fs.eventlog.YtEventLogFileSystem" -> "tech.ytsaurus.spyt.fs.eventlog.YtEventLogFileSystem",
      "tech.ytsaurus.misc.log.**" -> "tech.ytsaurus.misc.log.@1",
      "tech.ytsaurus.**" -> "shadedyandex.tech.ytsaurus.@1",
      "org.asynchttpclient.**" -> "shadedyandex.org.asynchttpclient.@1",
      "org.objenesis.**" -> "shadedyandex.org.objenesis.@1",
      "com.google.protobuf.**" -> "shadedyandex.com.google.protobuf.@1",
      "NYT.**" -> "shadedyandex.NYT.@1"
    ).inAll)

    val clientShadeRules: Seq[ShadeRule] = commonShadeRules ++ Seq(ShadeRule.rename(
      "tech.ytsaurus.spyt.format.GlobalTransactionSparkListener" -> "tech.ytsaurus.spyt.format.GlobalTransactionSparkListener",
      "tech.ytsaurus.spyt.format.ExtraOptimizationsSparkListener" -> "tech.ytsaurus.spyt.format.ExtraOptimizationsSparkListener",
      "org.objenesis.**" -> "shadeddatasource.org.objenesis.@1",
      "com.google.protobuf.**" -> "shadeddatasource.com.google.protobuf.@1",
      "NYT.**" -> "shadeddatasource.NYT.@1"
    ).inAll)

    def libraryRenamingShadingRule(source: String, targetName: String): MergeStrategy = {
      CustomMergeStrategy.rename {
        case dependency@(_: Project) => dependency.target
        case dependency@(_: Library) => Paths.get(source).getParent.resolve(targetName).toString
      }
    }

    lazy val printTestClasspath = taskKey[Unit]("")
    lazy val commonDependencies = settingKey[Seq[ModuleID]]("")
  }

  import autoImport._

  override def projectSettings: Seq[Def.Setting[_]] = Seq(
    externalResolvers := Resolver.combineDefaultResolvers(resolvers.value.toVector, mavenCentral = false),
    resolvers += Resolver.mavenLocal,
    resolvers += Resolver.mavenCentral,
    resolvers += ("YTsaurusSparkReleases" at "https://repo1.maven.org/maven2"),
    resolvers += ("YTsaurusSparkSnapshots" at "https://s01.oss.sonatype.org/content/repositories/snapshots/"),
    ThisBuild / version := (ThisBuild / spytVersion).value,
    organization := "tech.ytsaurus",
    name := s"spark-yt-${name.value}",
    organizationName := "YTsaurus",
    organizationHomepage := Some(url("https://ytsaurus.tech/")),
    scmInfo := Some(ScmInfo(url("https://github.com/ytsaurus/ytsaurus"), "scm:git@github.com:ytsaurus/ytsaurus.git")),
    developers := List(
      Developer("Alexvsalexvsalex", "Alexey Shishkin", "alex-shishkin@ytsaurus.tech", url("https://ytsaurus.tech/")),
      Developer("alextokarew", "Aleksandr Tokarev", "atokarew@ytsaurus.tech", url("https://ytsaurus.tech/")),
    ),
    description := "Spark over YTsaurus",
    licenses := List(
      "The Apache License, Version 2.0" -> new URL("http://www.apache.org/licenses/LICENSE-2.0.txt")
    ),
    homepage := Some(url("https://ytsaurus.tech/")),
    pomIncludeRepository := { _ => false },
    scalaVersion := "2.12.15",
    javacOptions ++= Seq("-source", "11", "-target", "11"),
    assembly / assemblyMergeStrategy := {
      case x if x endsWith "ahc-default.properties" => MergeStrategy.first
      case x if x endsWith "io.netty.versions.properties" => MergeStrategy.first
      case x if x endsWith "Log4j2Plugins.dat" => MergeStrategy.last
      case x if x endsWith "git.properties" => MergeStrategy.last
      case x if x endsWith "libnetty_transport_native_epoll_x86_64.so" => MergeStrategy.last
      case x if x endsWith "libnetty_transport_native_kqueue_x86_64.jnilib" => MergeStrategy.last
      case x =>
        val oldStrategy = (assembly / assemblyMergeStrategy).value
        oldStrategy(x)
    },
    assembly / assemblyOption := (assembly / assemblyOption).value.withIncludeScala(false),
    assembly / test := {},
    publishTo := {
      if (isSnapshot.value)
        Some("snapshots" at "https://s01.oss.sonatype.org/content/repositories/snapshots/")
      else
        Some("releases" at "https://s01.oss.sonatype.org/service/local/staging/deploy/maven2/")
    },
    publishMavenStyle := true,
    credentials += Credentials(Path.userHome / ".sbt" / ".credentials"),
    credentials += Credentials(Path.userHome / ".sbt" / ".ossrh_credentials"),
    libraryDependencies ++= testDeps,
    Test / fork := true,
    printTestClasspath := {
      (Test / dependencyClasspath).value.files.foreach(f => println(f.getAbsolutePath))
    },
    ThisBuild / spytSparkForkDependency := {
      Seq(
        "tech.ytsaurus.spark" %% "spark-core",
        "tech.ytsaurus.spark" %% "spark-sql"
      ).map(_ % sparkForkVersion).map(_ excludeAll
        ExclusionRule(organization = "org.apache.httpcomponents")
      ).map(_ % Provided)
    },
    commonDependencies := ytsaurusClient ++ (ThisBuild / spytSparkForkDependency).value ++ circe ++ logging.map(_ % Provided),
    Global / excludeLintKeys += commonDependencies,
    Global / pgpPassphrase := gpgPassphrase.map(_.toCharArray)
  )
}
